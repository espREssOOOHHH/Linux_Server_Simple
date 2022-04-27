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

Log &log=Log::Instance();
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

Http_connect::HTTP_CODE Http_connect::parse_headers()
{
    if('\0'==read_content[start_line_index])//find empty line
    {
        if(content_length_of_request!=0)
        {
            status_MainStateMachine=STATE_CONTENT;
            return NO_REQUEST;
        }
        else
            return GET_REQUEST;
    }
    else if(read_content.find("Connection:",start_line_index)<=1)//connection head
    {
        start_line_index+=sizeof "Connection:";
        if(read_content.find("keep-alive",start_line_index))
            keep_alive=true;
    }
    else if(read_content.find("Content-Length:",start_line_index)<=1)
    {
        start_line_index+=sizeof "Content-Length";
        content_length_of_request=atol(read_content.c_str()+start_line_index);
    }
    else if(read_content.find("Host:",start_line_index)<=1)
    {
        start_line_index+=sizeof "Host:";
        host_name=read_content.substr(start_line_index);
    }
    else if(read_content.find("User-Agent:")<=1)
    {
        start_line_index+=sizeof "User-Agent:";
        log.i("User-Agent:"+read_content.substr(start_line_index));
    }
    else
        log.e("unknown header :"+read_content.substr(start_line_index));
    
    return NO_REQUEST;
}

Http_connect::HTTP_CODE Http_connect::parse_content( )
{
    if(read_index>=content_length_of_request+checked_index)
    {
        read_content.resize(content_length_of_request);
        return GET_REQUEST;
    }
    else
        return NO_REQUEST;
}

Http_connect::HTTP_CODE Http_connect::parse_request_line()
{
    int pos=read_content.find(" ",start_line_index);
    if(pos==std::string::npos)
        return BAD_REQUEST;//no url
    int pos2=read_content.find(" ",start_line_index+pos);
    url=read_content.substr(pos+1,pos2-pos);

    if(read_content.find("GET")<=1)
        method=GET;
    else if(read_content.find("POST")<=1)
        {method=POST;return BAD_REQUEST;}
    else
        return BAD_REQUEST;

    http_version=read_content.substr(0,pos);
    if(pos<=1)
        return BAD_REQUEST;
    if(http_version!="HTTP/1.1")
        return BAD_REQUEST;
    if(url.find("http:/")==0)
        url=url.substr(sizeof "http:/");
    if(url.length()<1)
        return BAD_REQUEST;
    status_MainStateMachine=STATE_HEADER;
    return NO_REQUEST;
}
Http_connect::HTTP_CODE Http_connect::resolve()
{
    LINE_STATUS line_status=LINE_OK;
    HTTP_CODE ret=NO_REQUEST;

    while(
        (status_MainStateMachine==STATE_CONTENT and line_status==LINE_OK)
        or (line_status=parse_line())==LINE_OK)
    {
        start_line_index=checked_index;
        log.i("got 1 http line: "+std::string(read_content.begin()+start_line_index,read_content.end()));

        switch(status_MainStateMachine)
        {
            case STATE_REQUESTLINE:
            {
                if(parse_request_line()==BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case STATE_HEADER:
            {
                ret==parse_headers();
                if(BAD_REQUEST==ret)
                    return BAD_REQUEST;
                else if(GET_REQUEST==ret)
                    return do_request();

                break;
            }
            case STATE_CONTENT:
            {
                if(parse_content()==GET_REQUEST)
                    return do_request();
                line_status=LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

Http_connect::HTTP_CODE Http_connect::do_request()
{
    std::string temp=root_dir+url;
    filepath_buffer.assign(temp.begin(),temp.end());
    if(stat(&filepath_buffer[0],&file_status)<0)
        return NO_RESOURCE;
    if(!(file_status.st_mode & S_IROTH))
        return FOBBIDEN_REQUEST;
    if(S_ISDIR(file_status.st_mode))
        return BAD_REQUEST;
    
    int fd=open(&filepath_buffer[0],O_RDONLY);
    file_location=(char*)mmap(0,file_status.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    return FILE_REQUEST;
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

void Http_connect::unmap()
{
    if(file_location)
    {
        munmap( file_location, file_status.st_size );
        file_location = 0;
    }
}

bool Http_connect::write()
{
    
}