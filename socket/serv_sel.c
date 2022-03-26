#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<sys/select.h>
#include<ctype.h>

int main()
{
	struct sockaddr_in addr;
	int fd = socket(AF_INET,SOCK_STREAM,0);
	if (fd == -1)
	{
		perror("socket\n");
		return -1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9998);
	addr.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(fd,(const struct sockaddr*)&addr,sizeof(addr));
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
	FD_SET(fd,&readset);
	

	int max_fd = fd;
	while (1)
	{
		//����ԭʼ�Ķ����ļ�����������Ϊselect���ļ����������ܻ�仯
		fd_set tmp = readset;
		
		int ret = select(max_fd+1,&tmp,NULL,NULL,NULL);
		//�ж��ǲ��Ǽ�����fd  ���ǲ���1
		if (FD_ISSET(fd, &tmp))
		{
			struct sockaddr_in cliaddr;
			int cliLen = sizeof(cliaddr);
			//���ܿͻ��˵�����
			int cfd = accept(fd,(struct sockaddr*)&cliaddr,&cliLen); //�õ�һ��ͨѶ���ļ�������
			FD_SET(cfd,&readset);//��cfd���뱻���Ķ�������
			//��һ�α�select���òŻ����
			max_fd = cfd > max_fd ? cfd : max_fd;
		}
		for (int i = 0; i <= max_fd; i++)
		{
			//������Ļ���˵��i������ͨ�ŵ��ļ���������������select���ú�
			//�Ķ�������
			if (i != fd && FD_ISSET(i, &tmp))
			{
				//��������
				char buf[1024];
				//int len = recv(i, buf, sizeof(buf), 0);
				int len = read(i,buf,sizeof(buf));
				//printf("len is : %d \n",len);
				if (len > 0)
				{
					printf("client send : %s\n", buf);
					//send(i, buf, len, 0);
					write(i,buf,strlen(buf)+1);
				}
				else if (len == 0)
				{
					printf("client shutdown\n");
					FD_CLR(i,&readset);//�Ӷ����������
					close(i);
					break;
				}
				else
				{
					perror("recv");
					exit(1);
				}
				//printf("read buf = %s\n",buf);
				////Сдת��д
				//for (int j = 0; j < len; j++)
				//{
				//	buf[j] = toupper(buf[j]);
				//}
				//printf("after buf = %s\n", buf);

				//ret = send(i, buf, strlen(buf) + 1, 0);
				//if (ret == -1)
				//{
				//	printf("send error");
				//	exit(1);
				//}
			}
		}
		
	}
	close(fd);

	return 0;
}