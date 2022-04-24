#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <fstream>
#include <string.h>
#include <array>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 1024

using namespace std;
static bool run=true;

static const array<string,2> status_line{"200 OK","404 Not Found"};

static void handle_stop(int sig)
{
    run=false;
}

int main()
{

    signal(SIGINT,handle_stop);

    fstream address_config("address_config_v0.1.txt",ios::in);
    if(!address_config.is_open())
    {
        cout<<"config file does not exist!"<<endl;
        return -1;
    }

    string ip;
    address_config>>ip;
    int port;
    address_config>>port;
    int connection_number;
    address_config>>connection_number;
    string file_name;
    address_config>>file_name;
    

    cout<<"ip  "<<ip<<"  port:"<<port<<"  connection number max  "<<connection_number<<endl;

    sockaddr_in address;
    memset(&address,0,sizeof address);
    address.sin_family=AF_INET;//use ipv4
    inet_pton(AF_INET,ip.c_str(),&address.sin_addr);
    address.sin_port=htons(port);

    const int sock=socket(PF_INET,SOCK_STREAM,0);
    bind(sock,(struct sockaddr*)&address,sizeof address);
    listen(sock,connection_number);

    while(run){
    sockaddr_in client;
    socklen_t client_addrlength=sizeof client;
    int connfd=accept(sock,(sockaddr*)&client,&client_addrlength);

    if(connfd<0)
    {
        cout<<"error num is "<<errno<<endl;
        close(sock);
        return 0;
    }
    else
    {
        char *remote=new char[INET_ADDRSTRLEN];
        cout<<"connect with ip "<<inet_ntop(AF_INET,&client.sin_addr,remote,INET_ADDRSTRLEN)
            <<" and port "<<ntohs(client.sin_port)<<endl;
        
        char *buffer=new char[5000];
        memset(buffer,'\0',5000);
        //send(connfd,"hello\n",sizeof "hello",0);
        recv(connfd,buffer,4999,0);
        cout<<buffer<<endl;
        //send(connfd,"bye~\n",sizeof "bye~\n",0);

        bool valid=true;
        struct stat file_stat;
        if(stat(file_name.c_str(),&file_stat)<0)
        {
            valid=false;
        }


        int fd=open(file_name.c_str(),O_RDONLY);
        char* file_buf=new char[file_stat.st_size+1];
        memset(file_buf,'\0',file_stat.st_size+1);
        
        if(read(fd,file_buf,file_stat.st_size)<0)
        {
            valid=false;
        }

        int len=0;
        char* header_buf=new char[BUFFER_SIZE];

        if(valid)
        {
            len+=snprintf(header_buf,BUFFER_SIZE-1,"%s %s\r\n","HTTP/1.1",status_line[0].c_str());

            len+=snprintf(header_buf+len,BUFFER_SIZE-1-len,"%s","\r\n");

            struct iovec iv[2];
            iv[0].iov_base=header_buf;
            iv[0].iov_len=strlen(header_buf);
            iv[1].iov_base=file_buf;
            iv[1].iov_len=file_stat.st_size;
            writev(connfd,iv,2);
        }
        else
        {
            len+=snprintf(header_buf,BUFFER_SIZE-1,"%s %s\r\n","HTTP/1.1",status_line[1].c_str());
            len+=snprintf(header_buf+len,BUFFER_SIZE-1-len,"%s","\r\n");
            send(connfd,header_buf,strlen(header_buf),0);
        }
        
        close(connfd);
        delete[] file_buf;
    }  
    }
    close(sock);
    
    cout<<"quit successfully"<<endl;
    return 0;
}
