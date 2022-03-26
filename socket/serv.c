#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<string.h>

struct sockinfo
{
    struct sockaddr_in addr;
    int fd;
};

struct sockinfo infos[512];  //存放多个客户端的地址数据

void* serv_work(void* arg)
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
 //       printf("len is : %d \n",len);
        if (len > 0)
        {
            printf("client send : %s\n", buf);
            send(inf->fd, buf, len, 0);
        }
        else if (len == 0)
        {
            printf("client shutdown\n");
	    inf->fd=-1;
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }
    close(inf->fd); 
    //把数组位置再留出来
    inf->fd = -1;

    return NULL;
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
    //初始化结构体数组
    int max = sizeof(infos)/sizeof(infos[0]);
    for (int i = 0; i < max; i++)
    {
        __bzero(&infos[i],sizeof(infos[i]));
        infos[i].fd = -1;  //用于判断这个地方是不是空闲的
    }

    int caddr_len = sizeof(caddr);
    while (1)
  // for(int i=0;i<3;i++)
    {
        struct sockinfo* inf;
        for (int i = 0; i < max; i++)
        {
            if (infos[i].fd == -1)//可以用
            {
                inf = &infos[i]; //储存
                break;
            }
        }

        int cfd = accept(fd1, (struct sockaddr*)&inf->addr, &caddr_len);
        if (cfd == -1)
        {
            perror("accept\n");
            break;
        }
        //创建子线程
        pthread_t tid;
        inf->fd = cfd;

        pthread_create(&tid,NULL,serv_work,inf);
        //不想阻塞主线程来回收子线程，所以线程分离
        pthread_detach(tid);

    }
    //主线程 结束关闭通信的文件描述符
    close(fd1);

}
