#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<error.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<iostream>
#include"common.h"

using namespace std;

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUF_SIZE 0xFFFF
#define SERVER_WELCOME "Welcome you join to the chat room!Your char ID is Client #%d"
#define EXIT "EXIT"
#define CAUTION "There is only one int the chat room!"

#define OK "OK"

bool isClientwork=true;
char name[20];

//用于读取用户的输入，并将之转发给服务器
void*  writeMessage(void *argv)
{
    int sock=(long)argv;
    printf("--------用户界面----------\n");
    printf("-------请输入命令---------\n");
    printf("1.群发\n");
    printf("2.私聊\n");
    printf("3.显示当前在线用户\n");
    printf("4.退出\n");
    printf("-------------------------\n");

    while(isClientwork)
    {
        Msg sendmsg;
        char cmd=getchar();
        getchar();
        switch(cmd-'0')
        {
            case 1:
                strcpy(sendmsg.src,name);
                strcpy(sendmsg.dst,"All");
                printf("请输入群发信息：\n");
                fgets(sendmsg.buf,sizeof sendmsg.buf,stdin);
                sendmsg.type=2;
                send(sock,&sendmsg,sizeof sendmsg,0);
                break;
            case 2:
                strcpy(sendmsg.src,name);
                printf("请输入要私聊的用户的名称 以及聊天内容，以换行进行分割：\n");
                //接收方
                fgets(sendmsg.dst,sizeof sendmsg.dst,stdin);
                //聊天信息
                fgets(sendmsg.buf,sizeof sendmsg.buf,stdin);
                sendmsg.type=3;
                //发送消息
                send(sock,&sendmsg,sizeof sendmsg,0);
                break;
            case 3:
                sendmsg.type=4;
                send(sock,&sendmsg,sizeof sendmsg,0);
                break;
            case 4:
                close(sock);
                exit(0);
                break;
            default:
                printf("输入的命令有误，请重新输入!\n");
        }
    }
    return NULL;
}


int main()
{
    bzero(name,sizeof name);
    //设置服务器的IP地址和端口
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=PF_INET;
    serverAddr.sin_port=htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr=inet_addr(SERVER_IP);
    //建立socket
    int sock=socket(PF_INET,SOCK_STREAM,0);
    if(sock<0)
    {
        perror("scok error");
        exit(-1);
    }
    //连接服务器
    if(connect(sock,(struct sockaddr *)&serverAddr,sizeof serverAddr)<0)
    {
        perror("connection error");
        exit(-1);
    }

    bool isOK=false;
    printf("-------验证身份-----------\n");
    printf("1.注册\n");
    printf("2.登录\n");
    printf("3.退出\n");
    printf("-------------------------\n");
    //进行认证
    while(1)
    {
        char  c=getchar();
        getchar();
        Msg sendmsg,recmsg;
        int ret;
        switch(c-'0')
        {
            case 1:
                printf("-----请输入用户名和密码-----------\n");
                //scanf("%s",msg.src);
                //scanf("%s",msg.buf);
                //fgets(sendmsg.src,sizeof sendmsg.src,stdin);
                //fgets(sendmsg.buf,sizeof sendmsg.buf,stdin);
                printf("用户名：");
                scanf("%s",sendmsg.src);
                while(getchar()!='\n');
                printf("密码：");
                scanf("%s",sendmsg.buf);
                while(getchar()!='\n');

                //printf("帐号为: %s\n",sendmsg.src);
                //printf("密码为: %s\n",sendmsg.buf);

                sendmsg.type=0;
                ret=send(sock,&sendmsg,sizeof sendmsg,0);
                if(ret<0)
                {
                    perror("send error\n");
                }
                recv(sock,&recmsg,sizeof recmsg,0);
                if(strncasecmp(recmsg.buf,OK,sizeof OK)==0)
                {
                    printf("注册成功！\n");
                    strcpy(name,sendmsg.src);   //保存用户名
                    isOK=true;
                }
                else
                {//打印注册失败信息
                    printf("%s！\n",recmsg.buf);
                }
                break;
            case 2:
                printf("-----请输入用户名和密码-----------\n");
                printf("用户名：");

                scanf("%s",sendmsg.src);
                while(getchar()!='\n');
                printf("密码：");
                scanf("%s",sendmsg.buf);
                while(getchar()!='\n');

                //printf("帐号为: %s\n",sendmsg.src);
                //printf("密码为: %s\n",sendmsg.buf);

                sendmsg.type=1;
                ret=send(sock,&sendmsg,sizeof sendmsg,0);

                recv(sock,&recmsg,sizeof recmsg,0);
                if(strncasecmp(recmsg.buf,OK,sizeof OK)==0)
                {
                    printf("认证成功！\n");
                    strcpy(name,sendmsg.src);  //保存用户名
                    isOK=true;
                }
                else
                {//打印登录失败信息
                    printf("%s！\n",recmsg.buf);
                }
                break;
            case 3:
                close(sock);
                exit(0);
                break;
            default:
                printf("输入的信息有误，请重新输入！\n");
        }
        if(isOK==true && (c=='1' || c=='2'))
        {
            break;
        }
    }

    //线程实现读写分离
    pthread_t tid;
    pthread_create(&tid,NULL,writeMessage,(void*)(long)sock);
    pthread_detach(tid);

    //主进程读服务器发过来的信息，并进行打印
    Msg recvmsg;
    while(isClientwork)     //当标志为为true时，父进程在此循环
    {
        //收到服务器发送过来的消息
        //recv面向连接的，recvfrom不是面向连接的，当recvfrom中的addr为空时与recv基本一致
        int ret =recv(sock,&recvmsg,sizeof recvmsg,0);
        if(ret==0)
        {//服务器关闭连接
            printf("Server closed connection:%d\n",sock);
            close(sock);
            isClientwork=false;
        }
        else//打印消息
            printf("%s\n",recvmsg.buf);
    }
    return 0;
}
