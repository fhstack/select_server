#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define INIT -1

int startup(int port)
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    
    if(sock < 0)
    {
        perror("socket");
        return 2;
    }

    int opt = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR, &opt,sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock,(struct sockaddr*)&addr,sizeof(addr)) < 0)
    {
        perror("bind");
        return 2;
    }

    if(listen(sock,5) < 0)
    {
        perror("listen");
        return 2;
    }

    return sock;
}

void serviceIO(fd_set* rfds,int fd_array[],int num)
{
    int i = 0;
    for(;i < num;i++)
    {
        if(fd_array[i] == INIT)
            continue;

        //监听套接字时间处理
        if(i == 0 && FD_ISSET(fd_array[i],rfds))
        {
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            int sock = accept(fd_array[i],(struct sockaddr*)&sock,&len);

            if(sock < 0)
            {
                perror("accept");
                continue;
            }
            //客户端成功连接到服务器
            printf("get a new client\n");

            //找到一个位置放sock
            int j = 1;
            for(; j < num; j++)
            {
                if(fd_array[j] == INIT)
                    break;
            }
            //找到了一个可用位置
            if(j < num)
            {
                fd_array[j] = sock;
            }
            else//j == num
            {
                printf("fd_array is full!\n");
                close(sock);
                continue;
            }
            //不可以立即去读，因为客户端可能还并没有发数据
            //否则服务器就阻塞了
            continue;
        }

        //IO时间处理
        if(FD_ISSET(fd_array[i],rfds))
        {
            char buf[1024];
            ssize_t s = read(fd_array[i],buf,sizeof(buf)-1);
            if(s > 0)
            {
                buf[s] = 0;
                printf("%s",buf);
            }
            else if(s == 0)
            {
                printf("client quit");
                fd_array[i] = INIT;   
                close(fd_array[i]);    //直接关闭该位置
            }
            else
            {
                perror("read");
                fd_array[i] = INIT;
                close(fd_array[i]);
            }
        }
    }
}
int main(int argc,char* argv[])
{
    if(argc != 2)
    {
        perror("Usage: ./select_server [port]");
        return 1;
    }

    int fd_array[sizeof(fd_set)*8];
    int num = sizeof(fd_array)/sizeof(fd_array[0]);
    int listen_sock = startup(atoi(argv[1]));
    
    fd_array[0] = listen_sock;

    //读事件描述符集
    fd_set rfds;
    int maxfd = -1;
    int i = 1;
    for(;i < num;i++)
    {
        fd_array[i] = INIT;
    }

    for(;;)
    {
        struct timeval timeout = {1,0};
        
        for(i = 0; i < num; i++)
        {
            if(fd_array[i] == INIT)
                continue;

            FD_SET(fd_array[i],&rfds);
            if(maxfd < fd_array[i])
                maxfd = fd_array[i];
        }//更新maxfd rfds

        switch(select(maxfd+1,&rfds,NULL,NULL,/*&timeout*/ NULL))
        {
            case 0:
                printf("timeout\n");
                break;
            case -1:
                perror("selcet");
                break;
            default:
                serviceIO(&rfds,fd_array,num);
                break;
        }
    }
}
