#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <string>
#include <iostream>
#include <exception>

#include "log.h"
#include "mutex_lock.h"
#include "http_connect.h"
#include "threadpool.h"

const int MAX_FD =65536;
const int MAX_EVENT_NUMBER=10000;

Log& log=Log::Instance();

//show_error: you can use Http_connect::reply_internal_server_busy()
void addsig(int sig,void(handler)(int),bool restart=true)
{
    struct sigaction sa{};
    sa.sa_handler=handler;
    if(restart)
        sa.sa_flags|=SA_RESTART;
    sigfillset(&sa.sa_mask);

    if(sigaction(sig,&sa,nullptr)==-1)
        log.e("add sig error!");
}
int main(int arg_count,char* argv[])
{
    using std::string;
    if(arg_count<=2)
    {
        std::cout<<"arg error!"<<std::endl;
        return 1;
    }

    const string ip=argv[1];
    const int port=atoi(argv[2]);

    addsig(SIGPIPE,SIG_IGN);
    try
    {
        Threadpool<Http_connect> pool;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    
    

}