#include "http_connect.h"

//status string
const std::string OK_200_TITLE="200 OK";
const std::string ERROR_400_TITLE="400 Bad Request";
const std::string ERROR_400_FORM="The server cannot or will not process the request due to something that is perceived to be a client error\n";
const std::string ERROR_403_TITLE="403 Forbidden";
const std::string ERROR_403_FORM="The server understood the request, but will not fulfill it.\n";
const std::string ERROR_404_TITLE="404 Not Found";
const std::string ERROR_404_FORM="The requested file was not found on this server\n";
const std::string ERROR_500_TITLE="500 Internal Error";
const std::string ERROR_500_FORM="An error has occurred during connection to the server and that the requested page cannot be accessed.\n";

std::string Http_connect::root_dir="./";
int Http_connect::setnonblocking(int fd)
{
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void Http_connect::addfd(int epollfd,int fd,bool one_shot)
{
    epoll_event event;
    event.data.fd=fd;
    event.events=EPOLLIN | EPOLLET | EPOLLRDHUP;
    if(one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void Http_connect::removefd(int epollfd,int fd)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

void Http_connect::modfd(int epollfd,int fd,int event_)
{
    epoll_event event;
    event.data.fd=fd;
    event.events=event_ | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

int Http_connect::num_user=0;
int Http_connect::epollfd=-1;

void Http_connect::close(bool real_close)
{
    if(real_close and sock_fd!=-1)
    {
        removefd(epollfd,sock_fd);
        sock_fd=-1;
        num_user--;
    }
}

void Http_connect::init(int sockfd, const sockaddr_in& addr)
{
    sock_fd=sockfd;
    address=addr;

    /*int reuse=1;
    setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));*/

    addfd(epollfd,sockfd,true);
    num_user++;

    init();
}

void Http_connect::init()
{
    status_MainStateMachine=STATE_REQUESTLINE;
    keep_alive=false;

    method=GET;
    url="";
    http_version="";
    content_length_of_request=0;
    host_name="";
    start_line_index=0;
    checked_index=0;
    read_index=0;
    write_size=0;

    read_buffer.resize(READ_BUFFER_SIZE,'\0');
    write_buffer.resize(WRITE_BUFFER_SIZE,'\0');
    filepath_buffer.resize(FILENAME_MAX,'\0');
}

Http_connect::LINE_STATUS Http_connect::parse_line()
{
    while(checked_index<read_index)
    {
        if(read_content[checked_index]=='\r')
            if(1==read_index-checked_index)
                return LINE_OPEN;
            else if(read_content[checked_index+1]=='\n')
            {
                read_content[checked_index++]='\0';
                read_content[checked_index++]='\0';
                return LINE_OK;
            }
            else
                return LINE_BAD;
        else if(read_content[checked_index]=='\n')
            if(checked_index>1 and read_content[checked_index-1]=='\r')
            {
                read_content[checked_index++]='\0';
                read_content[checked_index++]='\0';
                return LINE_OK;
            }
            else
                return LINE_BAD;
        checked_index++;
    }

    return LINE_OPEN;
}

Http_connect::HTTP_CODE Http_connect::parse_headers(char* text)
{

}

Http_connect::HTTP_CODE Http_connect::parse_content( char* text )
{

}

Http_connect::HTTP_CODE Http_connect::parse_request_line(char* text)
{
    
}
Http_connect::HTTP_CODE Http_connect::resolve()
{

}

Http_connect::HTTP_CODE Http_connect::do_request()
{

}


bool Http_connect::read()
{
    int bytes_received=0;
    while(true)
    {
        read_buffer.assign(READ_BUFFER_SIZE,'\0');
        bytes_received=recv(sock_fd,&read_buffer[0],READ_BUFFER_SIZE,0);

        if(bytes_received==-1)
        {
            if(errno==EAGAIN or errno==EWOULDBLOCK)
                break;
            else
                return false;
        }
        else if(!bytes_received)
            return false;
    }
    read_content+=&read_buffer[0];
    read_index+=bytes_received;
}