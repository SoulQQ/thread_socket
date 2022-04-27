#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<unistd.h>
#include<string.h>

int main(int argc, char* argv[])
{
	if (argc<0)
	{
		printf("parameter is not enough\n");
		return 1;
	}
	struct sockaddr_in addr;
	__bzero(&addr,sizeof(addr));
	int lis_fd = socket(AF_INET,SOCK_STREAM,0);
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port = htons(8888);

	int ret = bind(lis_fd,(struct sockaddr *)&addr,sizeof(addr));
	//assert(ret != -1);

	if (listen(lis_fd, 128) == -1)
	{
		perror("listen\n");
		return -1;
	}

	while (1)
	{
		struct sockaddr_in cli_addr;
		socklen_t cli_len = sizeof(cli_addr);   //注意是socklen_t
		int con_fd = accept(lis_fd, (struct sockaddr*)&cli_addr,&cli_len);
		//printf("con_fd is :\n",con_fd);
		if (con_fd < 0)
		{
			printf("con_fd is %d:\n", con_fd);
			printf("accept connect error");
		}
		else
		{
			char request[1024];
			recv(con_fd, request, 1024, 0);
			request[strlen(request)+1]='\0';
			printf("%s \n",request);
			char buf[520] = "HTTP/1.1 200 ok\r\nconnection: close\r\n\r\n";//HTTP响应
			int s = send(con_fd, buf, strlen(buf), 0);//发送响应
			//printf("send=%d\n",s);
			int fd = open("hello.html", O_RDONLY);//消息体
			sendfile(con_fd, lis_fd, NULL, 2500);//零拷贝发送消息体
			close(lis_fd);
			close(con_fd);
		}
	}



    return 0;
}