#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <ctime>
#include <fstream>
#include <string>
#include <iomanip>
#include <queue>
#include <pthread.h>
#include <memory.h>
#include <memory>
#include <chrono>

#include "mutex_lock.h"

class Log {
    //thread safe,but slow
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
                std::string temp= time_now()+"  Debug: ";
                temp+=content;
                temp+="\n";
                locker.lock();
                messages.push(temp);
                locker.unlock();
                sem.signal();
                return 1;
            }
            else
                return 0;
        };

        int i(std::string content)//info
        {
            if(level>error)
            {
                std::string temp= time_now()+"  Info: ";
                temp+=content;
                temp+="\n";
                locker.lock();
                messages.push(temp);
                locker.unlock();
                sem.signal();
                return 1;
            }
            else
                return 0;
        };

        int e(std::string content)//error
        {
            std::string temp= time_now()+"  Error: ";
            temp+=content;
            temp+="\n";

            locker.lock();
            messages.push(temp);
            locker.unlock();
            sem.signal();
            return 1;
        }



    private:
        Log() {
            // Constructor code goes here.
        }

        void Init(){
            log_out.open(file_name,std::ios_base::app);
            stop=false;
            main_thread=new pthread_t;

            if(!log_out.is_open())
                std::cerr<<"Could not open log file!"<<std::endl,log_out.close();
            else if(pthread_create(main_thread,nullptr,handler,nullptr))
                std::cerr<<"Cannot create main thread!"<<std::endl,log_out.close();
            else
            {   
                pthread_detach(*main_thread);
                now=time(0);
                localTime=localtime(&now);

                std::string temp="\nServer start!\n now is ";
                temp+=time_now(1)+"\n";
                locker.lock();
                messages.push(temp);
                locker.unlock();
                sem.signal();
            }
        }

        ~Log() {
            // Destructor code goes here.
            stop=true;
            pthread_join(*main_thread,nullptr);
            while(!messages.empty())
            {
                log_out<<messages.front();
                messages.pop();
            }
            log_out<<"Server shutdown! \n final time is: "+time_now(1);
            log_out<<std::endl;
            log_out.close();
            delete main_thread;
        }

        // announce the friend class
        friend class Configer;

        enum Level{error=0,info,debug};

        std::string time_now(int date=0)//date=0:no year,month and day 
        {
            using std::chrono::system_clock;
            std::ostringstream stream;
            now=time(nullptr);
            localTime=localtime(&now);
            stream.imbue(std::locale("C.UTF-8"));

            
	        system_clock::time_point tp = system_clock::now();
            std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
            std::string milliseconds_str = std::to_string(ms.count() % 1000);\
            
	        if (milliseconds_str.length() < 3) 
		        milliseconds_str = std::string(3 - milliseconds_str.length(), '0') + milliseconds_str;
            if(date)
                stream<<std::put_time(localTime,"%Y-%m-%d %H:%M:%S");
            else
                stream<<std::put_time(localTime,"%H:%M:%S");
            
            return date?stream.str():stream.str()+"."+milliseconds_str;
        }

        static void* handler(void *arg)//main processing function
        {
            Log& thisLog=Log::Instance();
            while(!thisLog.stop)
            {
                thisLog.sem.wait();
                if(!thisLog.messages.empty())
                {
                    thisLog.log_out<<thisLog.messages.front();
                    thisLog.locker.lock();
                    thisLog.messages.pop();
                    thisLog.locker.unlock();
                }

            }
            return nullptr;
        };

        private:
                std::ofstream log_out;//日志文件输出流
                time_t now;//时间
                tm *localTime;//本地时间
                Level level=info;
                std::queue<std::string> messages;//消息队列
                std::string file_name;//文件名

                Semaphore sem;
                bool stop;
                Mutex_locker locker;
                pthread_t* main_thread;


};
#endif