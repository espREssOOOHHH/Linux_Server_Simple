#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <time.h>
#include <fstream>
#include <string>
#include <iomanip>
#include <queue>
#include <pthread.h>
#include <memory>
#include "mutex_lock.h"
#include <memory.h>

class Log {
    //usage: Log& e=Log::Instance();
    //e.operation();
    public:
        static Log& Instance() 
        {
        // Since it's a static variable, if the class has already been created,
        // it won't be created again.
        // And it **is** thread-safe in C++11.
        static Log myInstance;

        // Return a reference to our instance.
        return myInstance;
        }

        // delete copy and move constructors and assign operators
        Log(Log const&) = delete;             // Copy construct
        Log(Log&&) = delete;                  // Move construct
        Log& operator=(Log const&) = delete;  // Copy assign
        Log& operator=(Log &&) = delete;      // Move assign

        // Any other public methods.
        int d(std::string content)//debug
        {
            if(level>info)
            {
                pthread_t pr;

                std::string temp= time_now();
                temp+="  Debug: ";
                temp+=content;
                temp+="\n";
                char* pt=(char*)malloc(temp.size()*sizeof(char));
                memset(pt,'\0',sizeof(char)*temp.size());
                strcpy(pt,temp.c_str());
                pthread_create(&pr,nullptr,writer,pt);
                pthread_detach(pr);
                return 1;
            }
            else
                return 0;
        };

        int i(std::string content)//info
        {
            if(level>error)
            {
                pthread_t pr;

                std::string temp= time_now();
                temp+="  Info: ";
                temp+=content;
                temp+="\n";
                char* pt=(char*)malloc(temp.size()*sizeof(char));
                memset(pt,'\0',sizeof(char)*temp.size());
                strcpy(pt,temp.c_str());
                pthread_create(&pr,nullptr,writer,pt);
                pthread_detach(pr);
                return 1;
            }
            else
            return 0;
        };

        int e(std::string content)//error
        {
            pthread_t pr;

            std::string temp= time_now()+"  Error: ";
            temp+=content;
            temp+="\n";

            char* pt=(char*)malloc(temp.size()*sizeof(char));
            memset(pt,'\0',sizeof(char)*temp.size());
            strcpy(pt,temp.c_str());

            pthread_create(&pr,nullptr,writer,pt);
            pthread_detach(pr);
            return 1;
        }



    private:
        Log() {
            // Constructor code goes here.
            log_out.open("server.log",std::ios_base::app);
            stop=false;
            main_thread=new pthread_t;
            int return_val=pthread_create(main_thread,nullptr,handler,nullptr);
            pthread_detach(*main_thread);

            if(!log_out.is_open())
                std::cerr<<"Could not open log file!"<<std::endl,log_out.close();
            else if(return_val)
                std::cerr<<return_val<<"Cannot create main thread!"<<std::endl,log_out.close();
            else
            {   now=time(0);
                localTime=localtime(&now);

                std::string temp="\nServer start!\n now is ";
                temp+=time_now(1)+"\n";
                char* pt=(char*)malloc(temp.size()*sizeof(char));
                memset(pt,'\0',sizeof(char)*temp.size());
                strcpy(pt,temp.c_str());

                pthread_t pr;
                pthread_create(&pr,nullptr,writer,pt);
                pthread_detach(pr);
            }
        }

        ~Log() {
            // Destructor code goes here.
            stop=true;
            pthread_join(*main_thread,nullptr);
            log_out<<"Server shutdown! \n final time is: "+time_now(0);
            log_out<<std::endl;
            log_out.close();
            delete main_thread;
        }

        enum Level{error=0,info,debug};

        std::string time_now(int date=0)//date=0:no year,month and day 
        {
            std::ostringstream stream;
            now=time(nullptr);
            localTime=localtime(&now);
            std::cout.imbue(std::locale("C.UTF-8"));
            if(date)
                stream<<std::put_time(localTime,"%Y-%m-%d %H:%M:%S");
            else
                stream<<std::put_time(localTime,"%H:%M:%S");
            return stream.str();
        }

        static void* handler(void *arg);//main processing function
        static void* writer(void* arg);//write function
        // And any other protected methods.

        private:
                std::ofstream log_out;//日志文件输出流
                time_t now;//时间
                tm *localTime;//本地时间
                Level level=debug;
                std::queue<std::string> messages;//消息队列

                Semaphore sem;
                bool stop;
                Mutex_locker locker;
                pthread_t* main_thread;


};

void* Log::handler(void *arg)
{
    Log& thisLog=Log::Instance();
    while(!thisLog.stop)
    {
        thisLog.sem.wait();
        thisLog.locker.lock();
        if(!thisLog.messages.empty())
        {
            thisLog.log_out<<thisLog.messages.front();
            thisLog.messages.pop();
        }
        thisLog.locker.unlock();
    }
    return nullptr;
}

void* Log::writer(void *arg)
{
    std::string str((char*) arg);
    Log& thisLog=Log::Instance();
    
    thisLog.locker.lock();
    thisLog.messages.push(str);
    thisLog.locker.unlock();
    thisLog.sem.signal();

    pthread_exit(nullptr);
    free(arg);
    return nullptr;
}
#endif