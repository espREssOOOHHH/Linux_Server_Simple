#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <time.h>
#include <fstream>
#include <string>
#include <iomanip>

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
                time_now();
                log_out<<"  Debug: "<<content<<std::endl;
                return 1;
            }
            else
                return 0;
        };

        int i(std::string content)//info
        {
            if(level>error)
            {
                time_now();
                log_out<<"  Info: "<<content<<std::endl;
                return 1;
            }
            else
            return 0;
        };

        int e(std::string content)//error
        {
            time_now();
            log_out<<"  Error: "<<content<<std::endl;
            return 1;
        }



    private:
        Log() {
            // Constructor code goes here.
            log_out.open("server.log",std::ios_base::app);
            if(!log_out.is_open())
                std::cerr<<"Could not open log file!"<<std::endl,log_out.close();
            else
            {   now=time(0);
                localTime=localtime(&now);
                log_out<<"\nServer start!\n"<<"now is ";
                time_now(1);
                log_out<<std::endl;
            }
        }

        ~Log() {
            // Destructor code goes here.
            log_out<<"Server shutdown!"<<std::endl<<"final time is: ";
            time_now(0);
            log_out<<std::endl;
            log_out.close();
        }

        enum Level{error=0,info,debug};

        void time_now(int date=0)//date=0:no year,month and day 
        {
            now=time(nullptr);
            localTime=localtime(&now);
            std::cout.imbue(std::locale("C.UTF-8"));
            if(date)
                log_out<<std::put_time(localTime,"%Y-%m-%d %H:%M:%S");
            else
                log_out<<std::put_time(localTime,"%H:%M:%S");
            return ;
        }

        // And any other protected methods.

        private:
                std::ofstream log_out;//日志文件输出流
                time_t now;//时间
                tm *localTime;//本地时间
                Level level=debug;


};

#endif