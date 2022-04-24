#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <vector>
#include <exception>
#include <pthread.h>
#include <algorithm>
#include <iostream>

#include "mutex_lock.h"

template< class Function >
class threadpool
//This class is a thread pool working as half sync/half reactor mode.
{
	public:
		threadpool(threadpool&)=delete;
		threadpool& operator=(threadpool)=delete;

		//thread_number is the number of thread in the pool
		//max_requests is the max number of waiting requests.
		threadpool(int thread_number=8, int max_requests=16384);
		~threadpool();
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
		mutex_locker queue_locker;//mutex lock protecting the queue
		semaphore queue_status;//have task or not
		bool stop;//stop the thread
};


template<class Function>
threadpool<Function>::threadpool(int thread_number_,int max_requests_):
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
			throw std::exception();
		if(pthread_detach(*x))//detach this thread from main thread, return 0 when success
			throw std::exception();
	}
}
		
template<class Function>
threadpool<Function>::~threadpool()
{
	stop=true;
	for(auto x:threads)
		delete x;
}

template<class Function>
bool threadpool<Function>::append(Function* request)
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
	return true;
}

template<class Function>
void* threadpool<Function>::worker(void* arg)
{
	threadpool* pool=(threadpool*) arg;
	pool->run();
	return pool;
}

template<class Function>
void threadpool<Function>::run()
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
		(*request)();
	}
}
#endif
