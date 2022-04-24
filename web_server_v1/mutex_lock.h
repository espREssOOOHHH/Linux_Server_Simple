#ifndef MUTEX_LOCK_H
#define MUTEX_LOCK_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class semaphore
//This class can be used as semaphore.
{
	public:
		semaphore(semaphore&)=delete;
		semaphore& operator=(semaphore&)=delete;

		semaphore(int pshared=0,unsigned int value=0)
		//create a semaphore
		{
			if(sem_init(&m_sem,pshared,value))
				throw std::exception();
		}

		~semaphore()
		//destroy this semaphore
		{
			sem_destroy(&m_sem);
		}

		bool wait()
		//wait the semaphore (or P)
		{
			return !sem_wait(&m_sem);
		}

		bool signal()
		//signal the semaphore (or V)
		{
			return !sem_post(&m_sem);
		}

		bool post()
		{
			return !sem_post(&m_sem);
		}

		bool P()
		//alias to wait
		{
			return wait();
		}

		bool V()
		//alias to signal
		{
			return signal();
		}
		
		bool operator++(int)
		//increase the semaphore,such as sem++
		{
			return signal();
		}

		bool operator++()
		//increase the semaphore,such as ++sem
		{
			return signal();
		}

		bool operator--(int)
		//decrease the semaphore,such as sem--
		{
			return wait();
		}

		bool operator--()
		//decrease the semaphore,such as --sem
		{
			return wait();
		}
	private:
		sem_t m_sem;
};

class mutex_locker
//This class can be used as mutex lock.
{
	public:
		mutex_locker(mutex_locker&)=delete;
		mutex_locker& operator=(mutex_locker&)=delete;

		mutex_locker()
		//create a mutex lock
		{
			if(pthread_mutex_init(&m_mutex,nullptr))
				throw std::exception();
		}
		
		~mutex_locker()
		//destroy the mutex lock
		{
			pthread_mutex_destroy(&m_mutex);
		}

		bool lock()
		//get the lock
		{
			return !pthread_mutex_lock(&m_mutex);
		}

		bool wait()
		//wait for the lock
		{
			return lock();
		}

		bool P()
		//operation P
		{
			return lock();
		}

		bool unlock()
		//release the lock
		{
			return !pthread_mutex_unlock(&m_mutex);
		}

		bool V()
		//operation V
		{
			return unlock();
		}

		bool signal()
		//signal the lock
		{
			return unlock();
		}

		bool operator++(int)
		//increase the lock (unlock), such as lock++
		{
			return unlock();
		}

		bool operator++()
		//increase the lock (unlock),suck as ++lock
		{
			return unlock();
		}
		
		bool operator--(int)
		//decrease the lock (lock), such as lock--
		{
			return lock();
		}

		bool operator--()
		//decrease the lock (lock),suck as --lock
		{
			return lock();
		}


	private:
		pthread_mutex_t m_mutex;
};

class condition_var
//This class can be used as conditional variable.
{
	public:
		condition_var(condition_var&)=delete;
		condition_var& operator=(condition_var&)=delete;
		
		condition_var()
		//create a conditional variable
		{
			if(pthread_mutex_init(&m_mutex,nullptr))
				throw std::exception();
			if(pthread_cond_init(&m_cond,nullptr))
			{
				pthread_mutex_destroy(&m_mutex);
				throw std::exception();
			}
		}
		
		~condition_var()
		//destroy the conditional variable
		{
			pthread_mutex_destroy(&m_mutex);
			pthread_cond_destroy(&m_cond);
		}

		bool wait()
		//wait the cond, or add this thread to the queue
		{
			int ret=0;
			pthread_mutex_lock(&m_mutex);

			ret=pthread_cond_wait(&m_cond,&m_mutex);

			pthread_mutex_unlock(&m_mutex);
			return !ret;
		}

		bool P()
		//operation P
		{
			return wait();
		}

		bool signal()
		//signal the cond, or make one thread waiting for this cond wake
		{
			return !pthread_cond_signal(&m_cond);
		}

		bool signal_all()
		//signal the cond, or make all threads waiting for this cond wake
		{
			return !pthread_cond_broadcast(&m_cond);
		}

		bool V()
		//operation V
		{
			return signal();
		}

		bool operator++(int)
		//wait the cond, or add this thread to the queue,such as cond++
		{
			return wait();
		}

		bool operator++()
		//wait the cond, or add this thread to the queue,suck as ++cond
		{
			return wait();
		}

		bool operator--(int)
		//signal the cond, or make one thread waiting for this cond wake,such as cond--
		{
			return signal();
		}

		bool operator--()
		//signal the cond, or make one thread waiting for this cond wake,suck as --cond
		{
			return signal();
		}

	private:
		pthread_mutex_t m_mutex;
		pthread_cond_t m_cond;

};

#endif

