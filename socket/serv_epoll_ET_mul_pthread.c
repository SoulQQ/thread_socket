#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/select.h>
#include<ctype.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<errno.h>
#include<pthread.h>

typedef struct socketinfo
{
	int fd;  //可能是监听  可能是通信
	int epfd;
}socketinfo;

void* acceptConn(void* arg)
{
	printf(" accept pthread ID is： %ld\n",pthread_self());
	socketinfo * info = (socketinfo*)arg;
 	int cfd = accept(info->fd, NULL, NULL);
	//设置非阻塞属性  (边沿模式下)
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	struct epoll_event ev;
	//ev.events = EPOLLIN;
	ev.events = EPOLLIN | EPOLLET; //(边沿模式下)
	ev.data.fd = cfd;   //记录events的事件信息
	epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);   //返回-1失败  添加到epoll树上
	free(info);
	return NULL;
}
	
void* communication(void* arg)
{
	printf(" communication pthread ID is： %ld\n", pthread_self());
	socketinfo* info = (socketinfo*)arg;
	
	// 接收数据
	char buf[5];
	char temp[1024];
	__bzero(temp,sizeof(temp));
	//int len = recv(i, buf, sizeof(buf), 0);
	while (1)
	{
		int len = read(info->fd, buf, sizeof(buf));

		//printf("len is : %d \n",len);
		if (len > 0)
		{
			printf("client send : %s\n", buf);
			//send(i, buf, len, 0);

			write(info->fd, buf, strlen(buf) + 1);
		}
		else if (len == 0)
		{
			printf("client shutdown\n");
			//删掉  用于通信的文件描述符从epoll检测中删掉
			epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
			close(info->fd);
			break;
		}
		else
		{
			if (errno == EAGAIN)
			{
				printf("receive finished\n");
				send(info->fd, temp, strlen(temp) + 1, 0);  //字符串尾部是 \0
				break;
			}
			else
			{
				perror("recv");
				break;
			}
		}
		for (int i = 0; i < len; i++)
		{
			buf[i] = toupper(buf[i]);
			
		}
		strncat(temp + strlen(temp), buf, len);
		write(STDOUT_FILENO, buf, len);
		printf("\n");
	}
	free(info);
	return NULL;
}



int main()
{
	struct sockaddr_in addr;
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1)
	{
		perror("socket\n");
		return -1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9998);
	addr.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(lfd, (const struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind\n");
		return -1;
	}
	if (listen(lfd, 128) == -1)
	{
		perror("listen\n");
		return -1;
	}

	//创建epoll实例
	int epfd = epoll_create(1); //1的意思不是红黑树上的最大结点值是1，没有什么实际意义
	//只要指定一个大于0的数就行
	if (epfd == -1)
	{
		perror("epoll_create");
		exit(0);
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;   //记录events的事件信息
	//当ev的数据传给epoll_ctl之后，ev内的数据是会被拷贝的
	epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);   //返回-1失败  添加到epoll树上

	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(evs[0]);
	while (1)
	{
		int num = epoll_wait(epfd, evs, size, -1); //-1表示阻塞等待，检测就绪的总个数
		printf("num is %d\n", num);
		pthread_t tid;
		
		for (int i = 0; i < num; i++)
		{
			socketinfo* info = (socketinfo*)malloc(sizeof(socketinfo*));
			int fd = evs[i].data.fd;
			info->fd = fd;
			info->epfd = epfd;   //*****
			//判断是不是监听的fd
			if (fd == lfd)
			{
				pthread_create(&tid,NULL,acceptConn,info);
				pthread_detach(tid); //分离 ：子线程退出就不用主线程回收子线程的资源了
			}
			else   //不见监听的文件描述符  就是用于通信的文件描述符
			{
				pthread_create(&tid, NULL, communication, info);
				pthread_detach(tid);
			}
		}
	}

	close(lfd);

	return 0;
}