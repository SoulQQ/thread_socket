#include"ThreadPool.h"
#include<pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include"Task.h"
#include"ThreadPool.cpp"


void taskFunc(void* arg)
{
    int num = *(int*)arg;
    printf("thread %ld is working, number = %d\n",
        pthread_self(), num);
    sleep(1);
}

int main()
{
    // 创建线程池
    ThreadPool<int> pool(3,10);
    Task<int> task;
  
    for (int i = 0; i < 100; ++i)
    {
        int* num = new int(i+100);
        
        task.function = taskFunc;
        task.arg = num;
        pool.addTask(task);

    }

    sleep(30);
   

    return 0;
}