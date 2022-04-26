#ifndef HTTP_CONNECT_H
#define HTTP_CONNECT_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>

#include <string>
#include <pthread.h>
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <vector>

#include <sys/mman.h>

#include "mutex_lock.h"
#include "log.h"

class Http_connect
{
    public:
        //proprieties and state tags
        static const int MAX_LEN_FILENAME=200;//maxium length of filename
        static const int READ_BUFFER_SIZE=256;//size of read buffer
        static const int WRITE_BUFFER_SIZE=1024;//size of write buffer
        enum METHOD {GET=0,POST,HEAD,PUT,DELETE,
                    TRACE,OPTIONS,CONNECT,PATCH};//http request method
        enum STATE_CHECK {STATE_REQUESTLINE=0,STATE_HEADER,
                    STATE_CONTENT};//states of main state machine
        enum HTTP_CODE {NO_REQUEST=0,GET_REQUEST,BAD_REQUEST,
                    NO_RESOURCE,FOBBIDEN_REQUEST,FILE_REQUEST,
                    INTERNAL_ERROR,CLOSED_CONNECTION};//results for processing http request
        enum LINE_STATUS {LINE_OK=0,LINE_BAD,LINE_OPEN};//line reading status

        //other important constant
        static int epollfd;//epoll file descriptor
        static int num_user;//the number of users
        static std::string root_dir;//root directory of website

        //important function
        Http_connect(){};
        ~Http_connect(){};
        Http_connect(Http_connect &)=delete;
        Http_connect(Http_connect&&)=delete;
        Http_connect& operator=(Http_connect const &)=delete;
        Http_connect& operator=(Http_connect const &&)=delete;

        //methods
        void init(int sockfd, const sockaddr_in& addr);//init a new connection
        void close(bool real_close=true);//close the connection

        void operator()();//functor
        bool read();//non-blocking read
        bool write(HTTP_CODE ret);//non-blocking write

    private:
        void init();//init a new connection, hidden interface
        HTTP_CODE resolve();//resolve http request
        HTTP_CODE reply();//reply http request

        //these methods called by resolve();
        HTTP_CODE parse_request_line(char* text);
        HTTP_CODE parse_headers(char* text);
        HTTP_CODE parse_content(char* text);
        HTTP_CODE do_request();
        char* get_line() { return &read_buffer[start_line_index];};
        LINE_STATUS parse_line();

        //these methods called by reply();
        void unmap();
        bool add_response(const char* format,...);
        bool add_content(const char* content);
        bool add_status_line(int status,const char* title);
        bool add_headers(int content_length);
        bool add_content_length(int content_length);
        bool add_keep_alive();
        bool add_blank_line();

        //values and pointers
        int sock_fd;//socket of this http connection
        sockaddr_in address;//socket address of this http connection(the other side)
        std::vector<char> read_buffer;//read buffer
        std::string read_content;//read string for process
        int read_index;//hyper index of reading content in read_buffer
        int checked_index;//index of checking char in read_buffer
        int start_line_index;//index of resolving line
        std::vector<char> write_buffer;//write buffer
        int write_size;//number of bytes to be sent in write buffer
        STATE_CHECK status_MainStateMachine;//main state machine status
        METHOD method;//request method
        std::vector<char> filepath_buffer;//relative path of request file
        std::string url;//requested file name
        std::string http_version;//http protocal version
        std::string host_name;//host name
        int content_length_of_request;//request content length
        bool keep_alive;//whether keep alive
        char* file_location;//requested file location in memory
        struct stat file_status;//requested file status
            //these member are for writev
        struct iovec iv[2];
        int iv_count;//memory block count

    protected:
        int setnonblocking(int);//set file descriptor to nonblocking
        void addfd(int,int,bool);//add file descriptor to epoll set
        void removefd(int,int);//remove file descriptor from epoll set
        void modfd(int,int,int);//modify file descriptor in epoll set
};
#endif