#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <string>
#include <iostream>
#include <vector>
#include <exception>
#include <fstream>
#include <cstring>

#include "log.h"
#include "mutex_lock.h"
#include "http_connect.h"
#include "threadpool.h"
#include "configer.h"

const int MAX_FD =65536;
const int MAX_EVENT_NUMBER=10000;
bool run=true;//stop server variable

//show_error: you can use Http_connect::reply_internal_server_busy()
void addsig(int sig,void(*handler)(int),bool restart=true)
{
    Log& log=Log::Instance();
    struct sigaction sa{};
    sa.sa_handler=handler;
    if(restart)
        sa.sa_flags|=SA_RESTART;
    sigfillset(&sa.sa_mask);

    if(sigaction(sig,&sa,nullptr)==-1)
        log.e("add sig error!");
}
void signal_handler_stopServer(int sig)
{
    run=false;
    return;
}

int main()
{
    using std::string;
    using std::vector;
    using std::ios;
    //configer reading must be first
    Configer &conf=Configer::Instance();
    bool valid_conf=conf.load("server.conf");
    auto config_val=conf.load_main();
    if(!config_val.valid or !valid_conf)
        {std::cout<<"configer error!"<<std::endl;return -1;}

    static Log& log=Log::Instance();

    const int WAITING_NUM=5;//listen queue max num
    auto ip=config_val.ip;
    auto port=config_val.port;
    std::cout<<"ip  "<<ip<<"  port:"<<port<<std::endl;

    addsig(SIGPIPE,SIG_IGN);
    addsig(SIGINT,signal_handler_stopServer);

    Threadpool<Http_connect> pool(20,60000);

    vector<Http_connect> http_users(MAX_FD);
    //int user_count=0;
    
    int listenfd=socket(PF_INET,SOCK_STREAM,0);
    if(listenfd<0)
    {
        log.e("listenfd create error!");
        return -1;
    }
    
    struct linger tmp={1,0};
    setsockopt(listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof tmp);

    struct sockaddr_in address;
    std::memset(&address,0,sizeof address);
    address.sin_family=AF_INET;
    inet_pton(AF_INET,ip.c_str(),&address.sin_addr);
    address.sin_port=htons(port);

    if(bind(listenfd,(struct sockaddr*)&address,sizeof address)<0)
    {
        log.e("bind error!");
        return -1;
    }

    if(-1==listen(listenfd,WAITING_NUM))
    {
        log.e("listen error!");
        return -1;
    }

    vector<epoll_event> events(MAX_EVENT_NUMBER);
    int epollfd=epoll_create(5);
    if(epollfd==-1)
    {
        log.e("epoll fd create error!");
        return -1;
    }
    http_users[0].addfd(epollfd,listenfd,false);//just a method
    Http_connect::epollfd=epollfd;

    int num_epoll;
    while(run)
    {
        num_epoll=epoll_wait(epollfd,&events.at(0),MAX_EVENT_NUMBER,-1);
        if(num_epoll<0 and errno!=EINTR)
        {
            log.e("epoll faliure");
            break;
        }

        for(int i=0;i<num_epoll;i++)
        {
            int sockfd=events.at(i).data.fd;
            if(sockfd==listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength=sizeof client_address;
                int connfd=accept(listenfd,(struct sockaddr*)&client_address,
                                    &client_addrlength);
                if(connfd<0)
                {
                    log.e("errno is "+errno);
                    continue;
                }
                if(Http_connect::num_user>=MAX_FD)
                {   http_users.at(0).reply_internal_server_busy(connfd);
                    continue;
                }

                http_users.at(connfd).init(connfd,client_address);
                log.d("init a http_user");
            }
            else if(events.at(i).events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                http_users.at(sockfd).close_connection();
            else if(events.at(i).events & EPOLLIN)//read events
            {
                if(http_users.at(sockfd).read())
                    pool.append(&http_users.at(sockfd));
                else
                    http_users.at(sockfd).close_connection();
            }
            else if(events.at(i).events & EPOLLOUT)//write events
            {
                if(!http_users.at(sockfd).write())
                    http_users.at(sockfd).close_connection();
            }
            else
                continue;

        }
    }

    log.i("server shutdown");
    std::cout<<std::endl<<"Server Shutdown success"<<std::endl;
    close(epollfd);
    close(listenfd);
    return 0;
}