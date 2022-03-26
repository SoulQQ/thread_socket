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
		//保留原始的读的文件描述符，因为select后文件描述符可能会变化
		fd_set tmp = readset;
		
		int ret = select(max_fd+1,&tmp,NULL,NULL,NULL);
		//判断是不是监听的fd  看是不是1
		if (FD_ISSET(fd, &tmp))
		{
			struct sockaddr_in cliaddr;
			int cliLen = sizeof(cliaddr);
			//接受客户端的链接
			int cfd = accept(fd,(struct sockaddr*)&cliaddr,&cliLen); //得到一个通讯的文件描述符
			FD_SET(cfd,&readset);//把cfd放入被检测的读集合中
			//下一次被select调用才会更新
			max_fd = cfd > max_fd ? cfd : max_fd;
		}
		for (int i = 0; i <= max_fd; i++)
		{
			//都满足的话，说明i是用于通信的文件描述符，并且在select调用后
			//的读集合中
			if (i != fd && FD_ISSET(i, &tmp))
			{
				//接收数据
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
					FD_CLR(i,&readset);//从读集合中清除
					close(i);
					break;
				}
				else
				{
					perror("recv");
					exit(1);
				}
				//printf("read buf = %s\n",buf);
				////小写转大写
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