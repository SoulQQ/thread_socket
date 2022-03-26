#pragma once
#include"Task.h"
#include<iostream>
#include<string.h>
#include<unistd.h>
#include"Task.cpp"

template <typename T>
class ThreadPool
{
public:
	ThreadPool(int min, int max);
	~ThreadPool();

	//�������
	void* addTask(Task<T> task);
	//��ȡæ���߳���
	int getBusyNum();
	//��ȡ���ŵ��߳���
	int getLiveNum();
private:
	//�����̵߳�������
	static void* worker(void* arg);
	//�������̵߳�������
	static void* manager(void* arg);

	void threadExit();
private:
	pthread_mutex_t m_lock;
	pthread_cond_t m_notEmpty;
	pthread_t m_managerID;
	pthread_t* m_threadIDs;
	TaskQueue<T>* m_Task;
	int m_minNum;
	int m_maxNum;
	int m_busyNum;
	int m_liveNum;
	int m_exitNum;
	bool m_shutdown;

};