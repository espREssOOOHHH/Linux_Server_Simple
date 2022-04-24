#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <random>
#include <string>

#include "mutex_lock.h"
#include "threadpool.h"
#include "log.h"

Semaphore sem(0,1);
//Mutex_locker sem;

const int REPEAT_TIMES=5;
std::default_random_engine e;
std::uniform_real_distribution<double> u(0,3);

void* functor(void* arg)
{
	int rep=REPEAT_TIMES;
	while(rep--)
	{
		double sleep_time=u(e);
		sem.wait();
		std::cout<<"hello,my sequence is "<<1<<" and I will sleep @"<<sleep_time<<std::endl;
		sem.signal();
		sleep(sleep_time);
	}
	pthread_exit(nullptr);
}

void* functor2(void* arg)
{
	int rep=REPEAT_TIMES;
	while(rep--)
	{
		double sleep_time=u(e);
		sem.P();
		std::cout<<"hi,this functor uses P & V __"<<2<<" and I will sleep @"<<sleep_time<<std::endl;
		sem.V();
		sleep(sleep_time);
	}
	pthread_exit(nullptr);
}

void* functor3(void* arg)
{
	int rep=REPEAT_TIMES;
	while(rep--)
	{
		double sleep_time=u(e);
		sem--;
		std::cout<<"你好,my seq is "<<3<<" and I will sleep @"<<sleep_time<<std::endl;
		sem++;
		sleep(sleep_time);
	}
	pthread_exit(nullptr);
}
	std::string temp;
class func_class
{

	public:

		void operator() ()
		{
			sem--;
			temp+="task "+std::to_string(num)+": "+std::to_string(u(e))+'\t';
			sem++;
			return ;
		}
		int num;

		func_class(int num_=0):num(num_){};
};

int main(int arg_counts, char** argv)
{
	if(arg_counts==1 or argv[1][0]=='m')
	{
		Log& log=Log::Instance();
		pthread_t id1,id2,id3;
		pthread_create(&id1,nullptr,functor,nullptr);
		pthread_create(&id2,nullptr,functor2,nullptr);
		pthread_create(&id3,nullptr,functor3,nullptr);

		log.i("thread create success!");
		pthread_join(id1,nullptr);
		pthread_join(id2,nullptr);
		pthread_join(id3,nullptr);

		log.d("shut down");
				
	}
	else
	{
		func_class f1(1),f2(2),f3(3),f4(4),f5(5),f6(6);
		Threadpool<func_class> threadPool(3,10);
		auto show=[](func_class* f,Threadpool<func_class> &p){return p.append(f)?"true":"false";};
		std::cout<<show(&f1,threadPool)<<std::endl;
		std::cout<<show(&f2,threadPool)<<std::endl;
		std::cout<<show(&f3,threadPool)<<std::endl;
		std::cout<<show(&f4,threadPool)<<std::endl;
		std::cout<<show(&f5,threadPool)<<std::endl;
		std::cout<<show(&f6,threadPool)<<std::endl;
		sleep(1);
		temp+='\0';
		std::cout<<temp<<std::endl;		
	}
	
	return 0;
}


