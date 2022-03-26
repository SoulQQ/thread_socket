#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/select.h>
#include<ctype.h>
#include<pthread.h>

pthread_mutex_t mutex;

//�߳���Ҫ�Ķ������ɽṹ��
typedef struct fdinfo
{
	int fd;  //���� �� ͨ��
	//�����ַ
	int* maxfd;
	fd_set* rdset;
}FDinfo;

void* acceptConn(void* arg)
{
	printf("���̵߳�ID�� %ld\n\n",pthread_self());
	FDinfo* fdinf = (FDinfo*)arg;
	int cfd = accept(fdinf->fd, NULL, NULL); //�õ�һ��ͨѶ���ļ�������
	pthread_mutex_lock(&mutex);//�漰д������Ҫ����
	FD_SET(cfd, fdinf->rdset);//��cfd���뱻���Ķ�������
	//��һ�α�select���òŻ����
	*fdinf->maxfd = cfd > *fdinf->maxfd ? cfd : *fdinf->maxfd;
	pthread_mutex_unlock(&mutex);
	free(fdinf);
	return NULL;
}

void* communication(void* arg)
{
	printf("���̵߳�ID�� %ld\n\n", pthread_self());
	FDinfo* fdinf = (FDinfo*)arg;
	int cfd = accept(fdinf->fd, NULL, NULL); //�õ�һ��ͨѶ���ļ�������
	FD_SET(cfd, fdinf->rdset);//��cfd���뱻���Ķ�������
	//��һ�α�select���òŻ����
	*fdinf->maxfd = cfd > *fdinf->maxfd ? cfd : *fdinf->maxfd;

	char buf[1024];
	int len = recv(fdinf->fd, buf, sizeof(buf), 0);
	//printf("len is : %d \n",len);
	if (len > 0)
	{
		printf("client send : %s\n", buf);
		send(fdinf->fd, buf, len, 0);
	}
	else if (len == 0)
	{
		printf("client shutdown\n");
		pthread_mutex_lock(&mutex);
		FD_CLR(fdinf->fd, fdinf->rdset);//�Ӷ����������
		pthread_mutex_unlock(&mutex);
		close(fdinf->fd);
		return NULL;
	}
	else
	{
		perror("recv");
		free(fdinf);
		exit(1);
	}
	printf("read buf = %s\n", buf);
	//Сдת��д
	for (int j = 0; j < len; j++)
	{
		buf[j] = toupper(buf[j]);
	}
	printf("after buf = %s\n", buf);

	int ret = send(fdinf->fd, buf, strlen(buf) + 1, 0);
	if (ret == -1)
	{
		printf("send error");
		exit(1);
	}
	free(fdinf);
	return NULL;

}

int main()
{
	pthread_mutex_init(&mutex,NULL);

	struct sockaddr_in addr;
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		perror("socket\n");
		return -1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9998);
	addr.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(fd, (const struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1)
	{
		perror("bind\n");
		return -1;
	}
	if (listen(fd, 128) == -1)
	{
		perror("listen\n");
		return -1;
	}
	fd_set readset;
	FD_ZERO(&readset);
	FD_SET(fd, &readset);


	int max_fd = fd;
	while (1)
	{
		pthread_mutex_lock(&mutex);
		//����ԭʼ�Ķ����ļ�����������Ϊselect���ļ����������ܻ�仯
		fd_set tmp = readset;
		pthread_mutex_unlock(&mutex);
		int ret = select(max_fd + 1, &tmp, NULL, NULL, NULL);
		//�ж��ǲ��Ǽ�����fd  ���ǲ���1
		if (FD_ISSET(fd, &tmp))
		{
			//���ܿͻ��˵�����
			int cfd = accept(fd, NULL, NULL); //�õ�һ��ͨѶ���ļ�������
			FD_SET(cfd, &readset);//��cfd���뱻���Ķ�������
			//��һ�α�select���òŻ����
			max_fd = cfd > max_fd ? cfd : max_fd;

			//�������߳�
			pthread_t tid;
			FDinfo* inf = (FDinfo*)malloc(sizeof(FDinfo));
			inf->fd = fd;
			inf->maxfd = &max_fd;
			inf->rdset = &readset;
			pthread_create(&tid,NULL,acceptConn,inf);

			pthread_detach(tid);
		}
		for (int i = 0; i <= max_fd; i++)
		{
			//������Ļ���˵��i������ͨ�ŵ��ļ���������������select���ú�
			//�Ķ�������
			if (i != fd && FD_ISSET(i, &tmp))
			{
				//��������
				pthread_t tid;
				FDinfo* inf = (FDinfo*)malloc(sizeof(FDinfo));
				inf->fd = i;
				//inf->maxfd = &max_fd;
				inf->rdset = &readset;
				pthread_create(&tid, NULL, communication, inf);

				pthread_detach(tid);
				char buf[1024];
				int len = recv(i, buf, sizeof(buf), 0);
				//printf("len is : %d \n",len);
				if (len > 0)
				{
					printf("client send : %s\n", buf);
					send(i, buf, len, 0);
				}
				else if (len == 0)
				{
					printf("client shutdown\n");
					FD_CLR(i, &readset);//�Ӷ����������
					close(i);
					break;
				}
				else
				{
					perror("recv");
					exit(1);
				}
				printf("read buf = %s\n", buf);
				//Сдת��д
				for (int j = 0; j < len; j++)
				{
					buf[j] = toupper(buf[j]);
				}
				printf("after buf = %s\n", buf);

				ret = send(i, buf, strlen(buf) + 1, 0);
				if (ret == -1)
				{
					printf("send error");
					exit(1);
				}
			}
		}

	}
	close(fd);
	pthread_mutex_destroy(&mutex);

	return 0;
}