#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <time.h>

class Singleton {
    public:
        static Singleton& Instance() 
        {
            std::cout<<"hello"<<std::endl;
        // Since it's a static variable, if the class has already been created,
        // it won't be created again.
        // And it **is** thread-safe in C++11.
        static Singleton myInstance;

        // Return a reference to our instance.
        return myInstance;
        }

        // delete copy and move constructors and assign operators
        Singleton(Singleton const&) = delete;             // Copy construct
        Singleton(Singleton&&) = delete;                  // Move construct
        Singleton& operator=(Singleton const&) = delete;  // Copy assign
        Singleton& operator=(Singleton &&) = delete;      // Move assign

        // Any other public methods.

    protected:
        Singleton() {
            std::cout<<"hi"<<std::endl;
            // Constructor code goes here.
        }

        ~Singleton() {
            // Destructor code goes here.
        }

        // And any other protected methods.
};


class Log:public Singleton{

    public:
        Log()
        {
            std::cout<<"I am created!"<<std::endl;
        }

        void call(void)
        {
            std::cout<<"777";
        }

        ~Log()
        {
            std::cout<<"I will die!"<<std::endl;
        }
};

#endif