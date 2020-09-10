#include"server.h"
int main()
{//设置服务器的IP地址和端口号
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=PF_INET;
    serverAddr.sin_port=htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr=inet_addr(SERVER_IP);
    //创建监听套接字，TCP
    int listener=socket(PF_INET,SOCK_STREAM,0);
    if(listener<0)
    {
        perror("listener");
        exit(-1);
    }
    printf("listen socket created\n");
    //绑定IP地址和端口号
    if(bind(listener,(struct sockaddr *)&serverAddr,sizeof serverAddr)<0)
    {
        perror("blind error");
        exit(-1);
    }
    //监听套接字
    int ret=listen(listener,5);
    if(ret<0)
    {
        perror("listen error");
        exit(-1);
    }
    printf("Start to listen: %s\n",SERVER_IP);
    //创建epoll
    int epfd=epoll_create(EPOLL_SIZE);
    if(epfd<0)
    {
        perror("epfd error");
        exit(-1);
    }
    printf("epoll created,epollfd= %d\n",epfd);
    //定义epoll事件
    static struct epoll_event evts[EPOLL_SIZE];
    //将监听描述符加入epfd之中
    addfd(epfd,listener,true);

    while(1)
    {   //获取epoll事件和数量
        int epoll_events_count=epoll_wait(epfd,evts,EPOLL_SIZE,-1);
        if(epoll_events_count<0)
        {
            perror("epoll failure");
            break;
        }
        printf("epoll_event_count=%d \n",epoll_events_count);
        for(int i=0;i<epoll_events_count;i++)
        {//遍历epoll事件
            int sockfd=evts[i].data.fd;
            if(sockfd==listener)
            {//如果为监听描述符，表示有连接建立
                struct sockaddr_in client_address;
                socklen_t client_addrLenght=sizeof(struct sockaddr_in);
                //获取与客户端通信的描述符
                int clientfd =accept(listener,(struct sockaddr*)&client_address,&client_addrLenght);
                addfd(epfd,clientfd,true);          //添加到epoll监听集合中
                unAuthorized.insert(clientfd);      //插入未验证用户集合
            }
            else
            {
                //处理客户端的请求
                int ret=sendMessage(sockfd);
                if(ret<0)
                {
                    perror("error");
                    exit(-1);
                }
            }

        }
    }
    //关闭监听描述符和epoll描述符
    close(listener);
    close(epfd);
    return 0;
}

