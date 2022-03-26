#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include"ThreadPool.h"   //在linux中把该文件拷贝到socket目录下面了
//#include"../linux/ThreadPool.c"

void serv_work(void* arg);
void acceptConn(void* arg);

struct sockinfo
{
    struct sockaddr_in addr;
    int fd;
};

typedef struct poolinfo
{
    ThreadPool* po;
    int fd;
}poinfo;

struct sockinfo infos[512];  //存放多个客户端的地址数据


void serv_work(void* arg)
{
    struct sockinfo* inf = (struct sockinfo*)arg;
    //此时上面的ip和端口都是大端的，打印的时候转成小端
    char ip[32];
    printf("client ip is %s, port is %d \n\n", inet_ntop(AF_INET, &inf->addr.sin_addr.s_addr, ip, sizeof(ip)),
        ntohs(inf->addr.sin_port));

    while (1)
    {
        char buf[1024];
        int len = recv(inf->fd, buf, sizeof(buf), 0);
        //printf("len is : %d \n",len);
        if (len > 0)
        {
            printf("client send : %s\n", buf);
            send(inf->fd, buf, len, 0);
        }
        else if (len == 0)
        {
            printf("client shutdown\n");
            inf->fd = -1;
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }
    close(inf->fd); 
    ////把数组位置再留出来   不用数组了，因为已经分配堆内存了
    //inf->fd = -1;

}

void acceptConn(void* arg)
{
    int caddr_len = sizeof(struct sockaddr_in);
    poinfo *poin=(poinfo*)arg;
    while (1)
    {
        struct sockinfo* inf;
        inf = (struct sockinfo*)malloc(sizeof(struct sockinfo*));

        inf->fd = accept(poin->fd, (struct sockaddr*)&inf->addr, &caddr_len);
        if (inf->fd == -1)
        {
            perror("accept\n");
            break;
        }
        //添加通信的任务函数
        threadPoolAdd(poin->po,serv_work,inf); //inf堆信息传给serv_work函数   
    }
    close(poin->fd);
}

int main()
{
    int fd1 = socket(AF_INET,SOCK_STREAM,0);
    if (fd1 == -1)
    {
        perror("socket\n");
        return -1;
    }

    struct sockaddr_in saddr,caddr;
    

    saddr.sin_family = AF_INET;
    //必须是大端 换成网络字节序
    saddr.sin_port = htons(9999);
    saddr.sin_addr.s_addr = INADDR_ANY;   //0=0.0.0.0 任意
    int ret = bind(fd1, (struct sockaddr*)&saddr, sizeof(saddr));
    if (ret == -1)
    {
        perror("bind\n");
        return -1;
    }
    if (listen(fd1, 128) == -1)
    {
        perror("listen\n");
        return -1;
    }
    

    //创建线程池
    ThreadPool*pool= threadPoolCreate(3, 10, 100);
    poinfo* pinf = (poinfo*)malloc(sizeof(poinfo*));
    pinf->po = pool;
    pinf->fd = fd1;
    //添加任务 检测有没有新的客户端连接
    threadPoolAdd(pool,acceptConn,pinf);
    //主线程退出 不会影响其他线程的运行
    pthread_exit(NULL);

    return 0;

}