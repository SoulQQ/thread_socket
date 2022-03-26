#include"ThreadPool.h"
#include<pthread.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#define NUMBER 2  //ÿ����ӵ��̸߳���

//����ṹ��
typedef struct  Task
{
	void (*function)(void* arg);
	void* arg;
}Task;

// �̳߳ؽṹ��
struct ThreadPool
{
    // �������
    Task* taskQ;
    int queueCapacity;  // ����
    int queueSize;      // ��ǰ�������
    int queueFront;     // ��ͷ -> ȡ����
    int queueRear;      // ��β -> ������

    pthread_t managerID;    // �������߳�ID
    pthread_t* threadIDs;   // �������߳�ID
    int minNum;             // ��С�߳�����
    int maxNum;             // ����߳�����
    int busyNum;            // æ���̵߳ĸ���
    int liveNum;            // �����̵߳ĸ���
    int exitNum;            // Ҫ���ٵ��̸߳���
    pthread_mutex_t mutexPool;  // ���������̳߳�   ������
    pthread_mutex_t mutexBusy;  // ��busyNum����
    pthread_cond_t notFull;     // ��������ǲ�������   ��������
    pthread_cond_t notEmpty;    // ��������ǲ��ǿ���

    int shutdown;           // �ǲ���Ҫ�����̳߳�, ����Ϊ1, ������Ϊ0
};

ThreadPool* threadPoolCreate(int min, int max, int queueSize)
{

    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do
    {
        if (pool == NULL)
        {
            printf("malloc threadpool fail...\n");
            break;
        }

        pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if (pool->threadIDs == NULL)
        {
            printf("malloc threadIDs fail...\n");
            break;
        }
        memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;    // ����С�������
        pool->exitNum = 0;

        if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
            pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
            pthread_cond_init(&pool->notFull, NULL) != 0)
        {
            printf("mutex or condition init fail...\n");
            break;
        }

        // �������
        pool->taskQ = (Task*)malloc(sizeof(Task) * queueSize);
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        pool->shutdown = 0;

        // �����߳�
        pthread_create(&pool->managerID, NULL, manager, pool);

        for (int i = 0; i < min; ++i)
        {
            pthread_create(&pool->threadIDs[i], NULL, worker, pool);
        }
        return pool;
    } while (0);

    // �ͷ���Դ
    if (pool && pool->threadIDs) free(pool->threadIDs);
    if (pool && pool->taskQ) free(pool->taskQ);
    if (pool) free(pool);

    return NULL;


}

int threadPoolDestroy(ThreadPool* pool)
{
    if (pool == NULL)
    {
        return -1;
    }
    //�ر��̳߳�
    pool->shutdown = 1;
    //�������չ����߽���
    pthread_join(pool->managerID,NULL);
    //���������������߽���   ����worker
    for (int i = 0; i < pool->liveNum; ++i)
    {
        pthread_cond_signal(&pool->notEmpty);
    }
    // �ͷŶ��ڴ�
    if (pool->taskQ)
    {
        free(pool->taskQ);
    }
    if (pool->threadIDs)
    {
        free(pool->threadIDs);
    }

    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
    pool = NULL;

    return 0;
    return 0;
}

void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg)
{
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        //�����ߴﵽ������������������������̣߳�pthread_cond_wait��������while�У�
        //��pthread_cond_signalһ��ʹ�ã�����pthread_cond_signal����
        //&pool->notFull�ź�  �Ż������� 
        pthread_cond_wait(&pool->notFull,&pool->mutexPool);
    }
    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }
    //�������  �Ӷ�β����
    pool->taskQ[pool->queueRear].function = func;
    pool->taskQ[pool->queueRear].arg = arg;
    //ѭ������
    pool->queueRear = (pool->queueRear + 1) % (pool->queueCapacity);
    pool->queueSize++;

    //�ͷ�һ�����в�Ϊ�յ���������
    pthread_cond_signal(&pool->notEmpty);

    pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool* pool)
{
    //����ȡ�̳߳��е����� ����ð��̳߳���ס
    pthread_mutex_lock(&pool->mutexPool);
    int busynum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return busynum;
}

int threadPoolAliveNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int livenum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return livenum;
    return 0;
}

void* worker(void* arg)
{
    ThreadPool* pool = (ThreadPool*)arg;

    while (1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        //����������Ƿ�Ϊ��
        while (pool->queueSize == 0 && !pool->shutdown)
        {
            //�����������߳�  ��һ�� ������  ��threadPoolAddִ��ʱ������&pool->notEmpty
            //��pool->notEmpty���ˣ��ͻ���
            pthread_cond_wait(&pool->notEmpty,&pool->mutexPool);

            //�ж��ǲ���Ҫ�����߳�
            //�����  �����߳���ɱ
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                if (pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    threadExit(pool);
                }
            }
        }
        
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            threadExit(pool);
        }
        //�����������ȡ��һ������
        Task task;
        //��ͷ��ȡ����
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;
        //�ƶ�ͷ���
        pool->queueFront = (pool->queueFront + 1) % (pool->queueCapacity);
        pool->queueSize--;
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);

        printf("thread %ld start working ....\n",pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        task.function(task.arg);  //(*task.function)(task.arg); ����ִ�����
        //�ͷŶ��ڴ�
        free(task.arg);
        task.arg = NULL;

        printf("thread %ld finish working ....\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);

    }
    return NULL;
}

void* manager(void* arg)
{
    //void *  �������Ŀ�������������  ��������Ҫת��Ϊ������Ҫ��
    ThreadPool* pool = (ThreadPool*)arg;

    while (!pool->shutdown)
    {
        //ÿ������һ��
        sleep(3);

        //ȡ���̳߳�������������͵�ǰ�̵߳�����
        pthread_mutex_lock(&pool->mutexPool);
        int queuesize = pool->queueSize;
        int livenum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        //ȡ��æ���̵߳�����
        pthread_mutex_lock(&pool->mutexBusy);;
        int busynum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);

        //����߳�
        //����ĸ��� > �����̸߳���(����-æ)&& �����߳��� < ����߳���
        if (queuesize > livenum-busynum && livenum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int counter = 0;
            for (int i = 0; i < pool->maxNum && counter < NUMBER && pool->liveNum < pool->maxNum; i++)
            {
                if (pool->threadIDs[i] == 0)  //˵������߳��˳���
                {
                    pthread_create(&pool->threadIDs[i],NULL,worker,pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }
        // �����߳�
        // æ���߳�*2 < �����߳��� && �����߳�>��С�߳���
        if (busynum * 2 < livenum && livenum > pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            //�ù������߳���ɱ
            for (int i = 0; i < NUMBER; i++)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    return NULL;
}

void threadExit(ThreadPool* pool)
{
    pthread_t tid = pthread_self();
    for (int i = 0; i < pool->maxNum; i++)
    {
        //Ϊ�˽������߳�ʱ���߳��Ѿ��˳��������߳�ID����0������߳�ID��Ϊ0��
        //�������������� if (pool->threadIDs[i] == 0)
        if (pool->threadIDs[i] == tid)
        {
            pool->threadIDs[i] = 0;
            printf("threaddExit() called, %ld exiting ...\n",tid);
        }
    }
    pthread_exit(NULL);
}
