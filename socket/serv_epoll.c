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
	epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);   //返回-1失败  添加到epoll树上
	
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(evs[0]);
	while (1)
	{
		int num = epoll_wait(epfd, evs, size, -1); //-1表示阻塞等待，检测就绪的总个数
		printf("num is %d\n", num);
		for (int i = 0; i < num; i++)
		{
			int fd = evs[i].data.fd;
			//判断是不是监听的fd
			if (fd == lfd)
			{
				int cfd = accept(fd, NULL, NULL);
				//设置非阻塞属性  (边沿模式下)
				int flag = fcntl(cfd,F_GETFL);
				flag |= O_NONBLOCK;
				fcntl(cfd,F_SETFL,flag);
				//struct epoll_event ev;   因为第48行已经做了拷贝
				//ev.events = EPOLLIN;
				ev.events = EPOLLIN | EPOLLET; //(边沿模式下)
				ev.data.fd = cfd;   //记录events的事件信息
				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);   //返回-1失败  添加到epoll树上
			}
			else  //不见监听的文件描述符  就是用于通信的文件描述符
			{
				// 接收数据
				char buf[5];
				//int len = recv(i, buf, sizeof(buf), 0);
				while (1)
				{
					int len = read(fd, buf, sizeof(buf));

					//printf("len is : %d \n",len);
					if (len > 0)
					{
						printf("client send : %s\n", buf);
						//send(i, buf, len, 0);
						//char n_buf[1024];
						//fgets(n_buf,sizeof(n_buf),stdin);
						write(fd, buf, strlen(buf) + 1);
					}
					else if (len == 0)
					{
						printf("client shutdown\n");
						//删掉  用于通信的文件描述符从epoll检测中删掉
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
						break;
					}
					else
					{
						if (errno == EAGAIN)
						{
							printf("receive finished\n");
							break;
						}
						else
						{
							perror("recv");
							exit(1);
						}
						
					}
				}
			}
		}
	}

	close(lfd);

	return 0;
}