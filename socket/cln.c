#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

int main()
{
    int fd1 = socket(AF_INET, SOCK_STREAM, 0);
    if (fd1 == -1)
    {
        perror("socket\n");
        return -1;
    }

    struct sockaddr_in saddr,caddr;


    saddr.sin_family = AF_INET;
    //必须是大端 换成网络字节序
    saddr.sin_port = htons(9999);
    inet_pton(AF_INET,"192.168.1.188",&saddr.sin_addr.s_addr);
 

    
    int cfd = connect(fd1,(struct sockaddr*)&saddr, sizeof(saddr));
    if (cfd == -1)
    {
        perror("connect\n");
        return -1;
    }
    printf("connect success \n");
    //此时上面的ip和端口都是大端的，打印的时候转成小端
    char ip[32];
    //printf("client ip is %s, port is %d \n\n", inet_ntop(AF_INET, caddr.sin_addr.s_addr, ip, sizeof(ip)),
     //   ntohs(caddr.sin_port));
    int num = 0;
    while (1)
    {
        char buf[1024];
        sprintf(buf,"hello.... %d...\n",num++);
        //printf("client prepare send data \n");
        send(fd1,buf,strlen(buf)+1,0);
        //printf("client send data successful \n");
        memset(buf, 0, sizeof(buf));
        int len = recv(fd1, buf, sizeof(buf), 0);
        printf("receive length is %d\n",len);

        if (len > 0)
        {
            printf("server send : %s\n", buf);
            //send(cfd, buf, len, 0);
        }
        else if (len == 0)
        {
            printf("server shutdown\n");
            break;
        }
        else
        {
            perror("recv from server");
            break;
        }
        sleep(1);
    }

    close(fd1);
    


    return 0;
}