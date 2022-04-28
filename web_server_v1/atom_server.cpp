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

const int MAX_FD =65536;
const int MAX_EVENT_NUMBER=10000;


//show_error: you can use Http_connect::reply_internal_server_busy()
void addsig(int sig,void(handler)(int),bool restart=true)
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
int main()
{
    static Log& log=Log::Instance();
    using std::string;
    using std::vector;
    using std::ios;

    const int WAITING_NUM=5;//listen queue max num

    std::fstream address_config("address_config.txt",ios::in);
    if(!address_config.is_open())
    {
        std::cout<<"config file does not exist!"<<std::endl;
        return -1;
    }


    string ip;
    address_config>>ip;
    int port;
    address_config>>port;
    std::cout<<"ip  "<<ip<<"  port:"<<port<<std::endl;

    addsig(SIGPIPE,SIG_IGN);

    Threadpool<Http_connect> pool;

    vector<Http_connect> http_users(MAX_FD);
    int user_count=0;
    
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

    if(!listen(listenfd,WAITING_NUM)<0)
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

    while(true)
    {
        int num_epoll=epoll_wait(epollfd,&events[0],MAX_EVENT_NUMBER,-1);
        if(num_epoll<0 and errno!=EINTR)
        {
            log.e("epoll faliure");
            break;
        }

        for(int i=0;i<num_epoll;i++)
        {
            int sockfd=events[i].data.fd;
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
                {   http_users[0].reply_internal_server_busy(connfd);
                    continue;
                }

                http_users[connfd].init(connfd,client_address);
                log.d("init a http_user");
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                http_users[sockfd].close();
            else if(events[i].events & EPOLLIN)//read events
            {
                if(http_users[sockfd].read())
                    pool.append(&http_users[sockfd]);
                else
                    http_users[sockfd].close();
            }
            else if(events[i].events & EPOLLOUT)//write events
            {
                if(!http_users[sockfd].write())
                    http_users[sockfd].close();
            }
            else
                continue;

        }
    }

    log.i("server shutdown");
    close(epollfd);
    close(listenfd);
    return 0;
}