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
	int fd;  //�����Ǽ���  ������ͨ��
	int epfd;
}socketinfo;

void* acceptConn(void* arg)
{
	printf(" accept pthread ID is�� %ld\n",pthread_self());
	socketinfo * info = (socketinfo*)arg;
 	int cfd = accept(info->fd, NULL, NULL);
	//���÷���������  (����ģʽ��)
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	struct epoll_event ev;
	//ev.events = EPOLLIN;
	ev.events = EPOLLIN | EPOLLET; //(����ģʽ��)
	ev.data.fd = cfd;   //��¼events���¼���Ϣ
	epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);   //����-1ʧ��  ��ӵ�epoll����
	free(info);
	return NULL;
}
	
void* communication(void* arg)
{
	printf(" communication pthread ID is�� %ld\n", pthread_self());
	socketinfo* info = (socketinfo*)arg;
	
	// ��������
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
			//ɾ��  ����ͨ�ŵ��ļ���������epoll�����ɾ��
			epoll_ctl(info->epfd, EPOLL_CTL_DEL, info->fd, NULL);
			close(info->fd);
			break;
		}
		else
		{
			if (errno == EAGAIN)
			{
				printf("receive finished\n");
				send(info->fd, temp, strlen(temp) + 1, 0);  //�ַ���β���� \0
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

	//����epollʵ��
	int epfd = epoll_create(1); //1����˼���Ǻ�����ϵ������ֵ��1��û��ʲôʵ������
	//ֻҪָ��һ������0��������
	if (epfd == -1)
	{
		perror("epoll_create");
		exit(0);
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;   //��¼events���¼���Ϣ
	//��ev�����ݴ���epoll_ctl֮��ev�ڵ������ǻᱻ������
	epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);   //����-1ʧ��  ��ӵ�epoll����

	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(evs[0]);
	while (1)
	{
		int num = epoll_wait(epfd, evs, size, -1); //-1��ʾ�����ȴ������������ܸ���
		printf("num is %d\n", num);
		pthread_t tid;
		
		for (int i = 0; i < num; i++)
		{
			socketinfo* info = (socketinfo*)malloc(sizeof(socketinfo*));
			int fd = evs[i].data.fd;
			info->fd = fd;
			info->epfd = epfd;   //*****
			//�ж��ǲ��Ǽ�����fd
			if (fd == lfd)
			{
				pthread_create(&tid,NULL,acceptConn,info);
				pthread_detach(tid); //���� �����߳��˳��Ͳ������̻߳������̵߳���Դ��
			}
			else   //�����������ļ�������  ��������ͨ�ŵ��ļ�������
			{
				pthread_create(&tid, NULL, communication, info);
				pthread_detach(tid);
			}
		}
	}

	close(lfd);

	return 0;
}