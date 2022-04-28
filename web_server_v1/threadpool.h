#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <vector>
#include <exception>
#include <pthread.h>
#include <algorithm>
#include <iostream>

#include "log.h"
#include "mutex_lock.h"

template< class Function >
class Threadpool
//This class is a thread pool working as half sync/half reactor mode.
{
	public:
		Threadpool(Threadpool&)=delete;
		Threadpool& operator=(Threadpool)=delete;

		//thread_number is the number of thread in the pool
		//max_requests is the max number of waiting requests.
		Threadpool(int thread_number=8, int max_requests=16384);
		~Threadpool();
		//add task to the queue
		bool append(Function* request);

	private:
		//this function is for thread work
		static void* worker(void* arg);
		void run(); 
		
		int thread_number;//sum of thread in the pool
		int max_requests;//max requests of queue
		std::vector<pthread_t*> threads;//vector of threads
		std::queue< Function* > work_queue;//requests queue
		Mutex_locker queue_locker;//mutex lock protecting the queue
		Semaphore queue_status;//have task or not
		bool stop;//stop the thread

		Log& log=Log::Instance();
};


template<class Function>
Threadpool<Function>::Threadpool(int thread_number_,int max_requests_):
	thread_number(thread_number_),max_requests(max_requests_),stop(false)
{
	if(thread_number_<=0 or max_requests_<=0)
		throw std::exception();

	threads.resize(thread_number);
	
	std::for_each(threads.begin(),threads.end(),
			[](auto& t){t=new pthread_t;});

	for(pthread_t* x:threads)
	{
		if(pthread_create(x,nullptr,worker,this)!=0)
			log.e("thread create error!"),throw std::exception();
		if(pthread_detach(*x))//detach this thread from main thread, return 0 when success
			log.e("thread detach error!"),throw std::exception();
	}
	log.d("thread create success!");
}
		
template<class Function>
Threadpool<Function>::~Threadpool()
{
	stop=true;
	for(auto x:threads)
		delete x;
}

template<class Function>
bool Threadpool<Function>::append(Function* request)
{
	queue_locker.lock();
	if(work_queue.size()>max_requests)
	{
		queue_locker.unlock();
		return false;
	}
	work_queue.push(request);
	queue_locker.unlock();
	queue_status.signal();
	log.d("a task successfully append to the queue");
	return true;
}

template<class Function>
void* Threadpool<Function>::worker(void* arg)
{
	Threadpool* pool=(Threadpool*) arg;
	pool->run();
	return pool;
}

template<class Function>
void Threadpool<Function>::run()
{
	while(!stop)
	{
		queue_status.wait();
		queue_locker.lock();
		if(work_queue.empty())
		{
			queue_locker.unlock();
			continue;
		}
		
		Function* request=work_queue.front();
		work_queue.pop();
		queue_locker.unlock();
		if(!request)
			continue;
		log.d("a thread runs a task!");
		(*request)();
	}
}
#endif
