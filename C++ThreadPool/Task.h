#pragma once
#include<pthread.h>
#include<queue>
//定义任务结构体
using callback = void(*)(void*);
template <typename T>
struct Task
{
	Task()
	{
		function = nullptr;
		arg = nullptr;
	}

	Task(callback f, void* arg)
	{
		function = f;
		this->arg = (T*)arg;
	}

	callback function;
	T* arg;
};

template <typename T>
class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	void addTask(Task<T> task);
	void addTask(callback func,void * arg);
	//取出一个任务
	Task<T> takeTask();

	//获取任务个数
	int tasknum()
	{
		return m_queue.size();
	}

private:
	pthread_mutex_t m_mutex;
	std::queue<Task<T>> m_queue;

};