#include "http_connect.h"

//status string
const std::string OK_200_TITLE="OK";
const std::string OK_200_FORM="<html><body></body></html>";
const std::string ERROR_400_TITLE="Bad Request";
const std::string ERROR_400_FORM="The server cannot or will not process the request due to something that is perceived to be a client error\n";
const std::string ERROR_403_TITLE="Forbidden";
const std::string ERROR_403_FORM="The server understood the request, but will not fulfill it.\n";
const std::string ERROR_404_TITLE="Not Found";
const std::string ERROR_404_FORM="The requested file was not found on this server\n";
const std::string ERROR_500_TITLE="Server Internal Error";
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
    log.d("init a http connection");
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
    log.d("Http_connect::parse_headers() startLineIndex="+std::to_string(start_line_index)+read_content[start_line_index]);
    if('\0'==read_content[start_line_index] or start_line_index==read_content.length())//find empty line
    {
        if(method==GET)
            return GET_REQUEST;
        if(content_length_of_request!=0)
        {
            status_MainStateMachine=STATE_CONTENT;
            return NO_REQUEST;
        }
        else
            return GET_REQUEST;
    }
    else if(read_content.find("Connection:",start_line_index)-start_line_index<=1)//connection head
    {
        start_line_index+=sizeof "Connection:";
        if(read_content.find("keep-alive",start_line_index))
            keep_alive=true;
        log.d("Http_connect::parse_headers connection: "+std::to_string(keep_alive));
    }
    else if(read_content.find("Content-Length:",start_line_index)-start_line_index<=1)
    {
        start_line_index+=sizeof "Content-Length";
        content_length_of_request=atol(read_content.c_str()+start_line_index);
        log.d("Http_connect::parse_headers Content-Length: "+std::to_string(content_length_of_request));
    }
    else if(read_content.find("Host:",start_line_index)-start_line_index<=1)
    {
        start_line_index+=sizeof "Host:";
        host_name=read_content.substr(start_line_index,read_content.find('\0',start_line_index)-start_line_index);
        log.d("Http_connect::parse_headers Host: "+host_name);
    }
    else if(read_content.find("User-Agent:")-start_line_index<=1)
    {
        start_line_index+=sizeof "User-Agent:";
        log.i("User-Agent:"+read_content.substr(start_line_index,read_content.find('\0',start_line_index)-start_line_index));
    }
    else
        log.i("unknown header :"+read_content.substr(start_line_index,read_content.find('\0',start_line_index)-start_line_index));
        
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
    int pos=read_content.find(' ',start_line_index);
    if(pos==std::string::npos)
        return BAD_REQUEST;//no url
    int pos2=read_content.find(' ',start_line_index+pos+1);
    url=read_content.substr(pos+1,pos2-pos);

    log.d("Http_connect::parse_request_line: url="+url);
    if(read_content.find("GET")<=1)
        method=GET;
    else if(read_content.find("POST")<=1)
        {method=POST;return BAD_REQUEST;}
    else
        return BAD_REQUEST;

    if(read_content.find("HTTP/1.1")==std::string::npos)
        return BAD_REQUEST;
    else
        http_version="HTTP/1.1";

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
    log.d("Http_connect::resolve() status_MainStateMachine="+std::to_string(status_MainStateMachine));
    while(
        (status_MainStateMachine==STATE_CONTENT and line_status==LINE_OK)
        or (line_status=parse_line())==LINE_OK )
    {

        log.i("got 1 http line");

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
        start_line_index=checked_index;
    }
    log.d("main status="+std::to_string(status_MainStateMachine)+" line_status="+std::to_string(line_status));
    return NO_REQUEST;
}

Http_connect::HTTP_CODE Http_connect::do_request()
{
    std::string temp=root_dir+url;
    filepath_buffer.assign(temp.begin(),temp.end());
    if(stat(&filepath_buffer[0],&file_status)<0)
        return NO_RESOURCE;
    if(!(file_status.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;
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

        log.d("Http_connect::read() received "+std::to_string(bytes_received)+" bytes");
        if(bytes_received==-1)
        {
            if(errno==EAGAIN or errno==EWOULDBLOCK)
                break;
            else
                return false;
        }
        else if(!bytes_received)
            return false;

        read_content+=&read_buffer[0];
        read_index+=bytes_received;
    }
    return true;
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
    log.d("Http_connect::write() write_size="+std::to_string(write_size));
    if(!write_size)
    {
        modfd(epollfd,sock_fd,EPOLLIN);
        init();
        return true;
    }

    int ret_val;
    int bytes_to_be_send=write_size;
    int bytes_already_send;
    
    while(1)
    {
        ret_val=writev(sock_fd,iv,iv_count);
        if(ret_val<=-1)//if TCP write buffer is full, wait for next EPOLLOUT event
        {
            if(errno=EAGAIN)
            {
                modfd(epollfd,sock_fd,EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }
        log.d("Http_connect::write() write_size="+std::to_string(write_size));
        bytes_to_be_send-=ret_val;
        bytes_already_send+=ret_val;
        if(bytes_to_be_send<=bytes_already_send)
        {
            //send success
            unmap();
            if(keep_alive)
            {
                init();
                modfd(epollfd,sock_fd,EPOLLIN);
                return true;
            }
            else
            {
                modfd(epollfd,sock_fd,EPOLLIN);
                return false;
            }
        }

    }
}

bool Http_connect::add_response(const std::string content)
{
    if(write_size>=WRITE_BUFFER_SIZE or content.size()+write_size>=WRITE_BUFFER_SIZE)
        return false;
    
    copy(content.begin(),content.end(),write_buffer.begin()+write_size);
    write_size+=content.length();
    log.d("Http_connect::add_response write_size="+std::to_string(write_size));
    return true;
}

bool Http_connect::add_status_line(const std::string status,const std::string title)
{
    return add_response("HTTP/1.1 "+status+" "+title+"\r\n");
}

bool Http_connect::add_headers(int length)
{
    add_content_length(length);
    add_keep_alive();
    add_blank_line();
    return true;
}

bool Http_connect::add_content_length(int length)
{
    return add_response("Content-Length: "+std::to_string(length)+"\r\n");
}

bool Http_connect::add_keep_alive()
{
    return add_response("Connection: "+(keep_alive)?"keep-alive":"close");
}

bool Http_connect::add_blank_line()
{
    return add_response("\r\n");
}

bool Http_connect::add_content(const std::string content)
{
    return add_response(content);
}

void Http_connect::reply_internal_server_busy(int connfd)
{
    log.e("error 500 : Internal server busy");
    std::string info="HTTP/1.1 500 "+ERROR_500_TITLE+"\r\n";
    info+="Content-Length: "+std::to_string(ERROR_500_FORM.length());+"\r\n";
    info+=ERROR_500_FORM;
    send(connfd,info.c_str(),info.size(),0);
    close(connfd);

}
bool Http_connect::reply(HTTP_CODE ret)
{
    auto operation=[this](std::string num,std::string title,std::string form){
        this->add_status_line(num,title);
        this->add_headers(form.length());
        if(!this->add_content(form))
            return false;
        return true;
    };
    switch(ret)
    {
        case INTERNAL_ERROR:
        {
            log.i("http code 500 : Internal error");
            if(!operation(std::string("500"),ERROR_500_TITLE,ERROR_500_FORM))
                return false;
            break;
        }
        case BAD_REQUEST:
        {
            log.i("http code 400 : Bad request");
            if(!operation(std::string("400"),ERROR_400_TITLE,ERROR_400_FORM))
                return false;
            break;
        }
        case NO_RESOURCE:
        {
            log.i("http code 404 : Not found");
            if(!operation(std::string("404"),ERROR_404_TITLE,ERROR_404_FORM))
                return false;
            break;

        }
        case FORBIDDEN_REQUEST:
        {
            log.i("http code 403 : Request forbidden");
            if(!operation(std::string("403"),ERROR_403_TITLE,ERROR_403_FORM))
                return false;
            break;
        }
        case FILE_REQUEST:
        {
            log.i("http code 200 : OK");
            add_status_line("200",OK_200_TITLE);
            if(file_status.st_size)
            {
                log.d("web document founded");
                add_headers(file_status.st_size);
                iv[0].iov_base=&write_buffer[0];
                iv[0].iov_len=write_size;
                iv[1].iov_base=file_location;
                iv[1].iov_len=file_status.st_size;
                iv_count=2;
                return true;
            }
            else
            {
                log.d("web document not found");
                add_headers(OK_200_FORM.length());
                if(!add_content(OK_200_FORM));
                    return false;
            }
        }
        default:
            return false;
    }

    iv[0].iov_base=&write_buffer[0];
    iv[0].iov_len=write_size;
    iv_count=1;
    return true;
}

void Http_connect::operator()()
{
    log.d("start resolve");
    const auto return_val=resolve();
    if(NO_REQUEST==return_val)
    {
        log.d("no request");
        modfd(epollfd,sock_fd,EPOLLIN);
        return;
    }

    if(!reply(return_val))
    {
        log.d("reply not success");
        close();
    }

    log.d("reply success");
    modfd(epollfd,sock_fd,EPOLLOUT);
}