#include"ThreadPool.h"
#include"Task.h"

using namespace std;
#define  NUMBER 3 

template <typename T>
ThreadPool<T>::ThreadPool(int min, int max)
{
	m_Task = new TaskQueue<T>;
	do
	{
		m_minNum = min;
		m_maxNum = max;
		m_busyNum = 0;
		m_liveNum = m_minNum;
		m_exitNum = 0;

		m_threadIDs = new pthread_t[m_maxNum];
		if (m_threadIDs == nullptr)
		{
			cout << "malloc failed...\n" << endl;
		}
		memset(m_threadIDs,0,sizeof(pthread_t)*m_maxNum);

		if (pthread_mutex_init(&m_lock, NULL) != 0 ||
			pthread_cond_init(&m_notEmpty,NULL) != 0)
		{
			printf("mutex or condition init fail...\n");
			break;
		}

		//创建线程   这个this？？？？
		pthread_create(&m_managerID, NULL, worker, this);

		for (int i = 0; i < m_minNum; i++)
		{
			pthread_create(&m_threadIDs[i], NULL, manager, this);
		}
	} while (0);
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
	m_shutdown = 1;
	//阻塞回收管理者进程
	pthread_join(m_managerID,NULL);
	//唤醒阻塞的消费者进程
	for (int i = 0; i < m_liveNum; i++)
	{
		pthread_cond_signal(&m_notEmpty);
	}

	if (m_Task)  delete m_Task;
	if (m_threadIDs) delete[]m_threadIDs;
	pthread_mutex_destroy(&m_lock);
}


template <typename T>
void* ThreadPool<T>::addTask(Task<T> task)
{
	if (m_shutdown)
	{
		return nullptr;
	}
	m_Task->addTask(task);
	pthread_cond_signal(&m_notEmpty);
	return nullptr;
}

template <typename T>
int ThreadPool<T>::getBusyNum()
{
	pthread_mutex_lock(&m_lock);
	int busynum = m_busyNum;
	pthread_mutex_unlock(&m_lock);
	return busynum;
}

template <typename T>
int ThreadPool<T>::getLiveNum()
{
	pthread_mutex_lock(&m_lock);
	int livenum = m_liveNum;
	pthread_mutex_unlock(&m_lock);
	return livenum;
}

template <typename T>
void* ThreadPool<T>::worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*> (arg);
	while (true)
	{
		pthread_mutex_lock(&pool->m_lock);
		if (pool->m_Task->tasknum() == 0 && !!pool->m_shutdown)
		{
			pthread_cond_wait(&pool->m_notEmpty,&pool->m_lock);
			if (pool->m_exitNum > 0)
			{
				pool->m_exitNum--;
				if (pool->m_liveNum < pool->m_minNum)
				{
					pool->m_liveNum--;
					pthread_mutex_unlock(&pool->m_lock);
					pool->threadExit();
				}
			}
		}
		if (pool->m_shutdown)
		{
			pthread_mutex_unlock(&pool->m_lock);
			pool->threadExit();
		}

		Task<T> task = pool->m_Task->takeTask();
		pool->m_busyNum++;
		pthread_mutex_unlock(&pool->m_lock);
		cout << "thread" << to_string(pthread_self()) << "start working..." << endl;
		//task.arg = new ThreadPool*;
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;

		cout << "thread" << to_string(pthread_self()) << "end working..." << endl;
		pthread_mutex_lock(&pool->m_lock);
		pool->m_busyNum--;
		pthread_mutex_unlock(&pool->m_lock);
		
	}

	return nullptr;
}

template <typename T>
void* ThreadPool<T>::manager(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*> (arg);
	while (!pool->m_shutdown)
	{
		sleep(5);
		pthread_mutex_lock(&pool->m_lock);
		int queuesize = pool->m_Task->tasknum();
		int livenum = pool->m_liveNum;
		int busynum = pool->m_busyNum;
		pthread_mutex_unlock(&pool->m_lock);

		if (queuesize > livenum && livenum < pool->m_maxNum)
		{
			pthread_mutex_lock(&pool->m_lock);
			int num = 0;
			for (int i = 0; num < NUMBER && i<pool->m_maxNum && pool->m_liveNum<pool->m_maxNum; i++)
			{
				if (pool->m_threadIDs[i] == 0)
				{
					pthread_create(&pool->m_threadIDs[i],NULL,worker,pool);
					num++;
					pool->m_liveNum++;
				}
				
			}
			pthread_mutex_unlock(&pool->m_lock);
		}

		if (busynum * 2 < livenum && livenum > pool->m_minNum)
		{
			pthread_mutex_lock(&pool->m_lock);
			pool->m_exitNum = NUMBER;
			pthread_mutex_unlock(&pool->m_lock);
			for (int i = 0; i < NUMBER; ++i)
			{
				pthread_cond_signal(&pool->m_notEmpty);
			}
		}

	}
	return nullptr;
}

template <typename T>
void ThreadPool<T>::threadExit()
{
	pthread_t tid = pthread_self();
	for (int i=0;i<m_maxNum;i++)
	{
		if (m_threadIDs[i] == tid)
		{
			cout << "threadExit() function: thread "
				<< to_string(pthread_self()) << " exiting..." << endl;
			m_threadIDs[i] = 0;
			break;
		}
	}
	pthread_exit(NULL);
}
