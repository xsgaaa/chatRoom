#ifndef UTILITY_H
#define UNILITY_H

#include<iostream>
#include<list>
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
#include<unordered_map>
#include<unordered_set>
//mysql头文件
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/driver.h>
#include <jdbc/mysql_error.h>
#include <jdbc/cppconn/driver.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/cppconn/prepared_statement.h>

#include"common.h"
using namespace std;
using namespace sql;
//client_list存放已经验证过的用户
unordered_set<int> client_list;
//unAuthorized存放未验证的用户
unordered_set<int> unAuthorized;



#define DBHOST "tcp://127.0.0.1:3306"
#define USER "root"
#define PASSWORD "123456"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define EPOLL_SIZE 5000
#define BUF_SIZE 0xFFFF

#define SERVER_WELCOME "Welcome you join to the chat room!Your char ID is Client #%d"
#define EXIT "EXIT"
#define CAUTION "There is only one int the chat room!"

//模仿密码
//unordered_map<string,string> table={{"zhangsan","zhangsan"},{"lisi","lisi001"},{"wangwu","wangwu1234"},{"wangdachui","123456"}};
unordered_map<int,string> fdToname;
unordered_map<string,int> nameTofd;


/*
    * 设置非阻塞
*/
int setnonblocking(int sockfd)
{
    fcntl(sockfd,fcntl(sockfd,F_GETFD,0)|(O_NONBLOCK));
    return 0;
}

/*
    * 将fd添加到epollfd中，并且设置ET
*/
void addfd(int epollfd,int fd,bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd=fd;
    ev.events=EPOLLIN;      //epoll监听读事件
    if(enable_et)   ev.events=EPOLLIN|EPOLLET;      //设置边沿出发ET
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);
    setnonblocking(fd);     //将文件描述符设置为非阻塞
    printf("fd added to epoll!\n\n");
}

/*
    *将客户端发送的消息以广播的形式，发送给其他客户端
*/
int sendMessage(int clientfd)
{
    Msg recvmsg,sendmsg;
    int len=recv(clientfd,&recvmsg,sizeof recvmsg,0);

    Driver *driver=get_driver_instance();
    Connection *conn=driver->connect(DBHOST,USER,PASSWORD);
    Statement *stmt=conn->createStatement();
    stmt->execute("SET NAMES utf8");
    stmt->execute("USE mydatabase");
    ResultSet * resultSet;

    if(len==0)
    {
        //关闭与之通信的文件描述符
        close(clientfd);
        //消息长度为0,客户端已经关闭了socket文件描述符
        if(client_list.count(clientfd))
        {
            client_list.erase(clientfd);   //集合之中删除掉上述描述符
            nameTofd.erase(fdToname[clientfd]);
            fdToname.erase(clientfd);
            printf("ClientID =%d closed.\n Now there are %d client in the chat room",clientfd,(int)client_list.size());
        }
        else
        {
            unAuthorized.erase(clientfd);
        }
    }
    else
    {
        string name,password,query="select password from info where name=\"";
        int i=0,ret=-1,col_count=0;
        //有消息收到
        if(recvmsg.type==0)
        {
            string ().swap(name);
            string ().swap(password);
            i=0;
            while(recvmsg.src[i]!='\0') //保存用户名
                name.push_back(recvmsg.src[i++]);
            i=0;
            while(recvmsg.buf[i]!='\0') //保存密码
                password.push_back(recvmsg.buf[i++]);
            query+=name;
            query+="\";";

            resultSet=stmt->executeQuery(query);    //改为你的库和表
            col_count=resultSet->getMetaData()->getColumnCount();

            //查询用户名不再数据库中
            if(!resultSet->next())
            {
                //table[name]=password;
                strcpy(sendmsg.src,"server");
                strcpy(sendmsg.buf,"OK");
                sendmsg.type=0;
                send(clientfd,&sendmsg,sizeof sendmsg,0);

                string sqlinsr="insert into info (name,password) values(\"";

                sqlinsr+=name;
                sqlinsr+="\",\"";
                sqlinsr+=password;
                sqlinsr+="\");";

                stmt->execute(sqlinsr);

                client_list.insert(clientfd);   //插入就绪集合
                unAuthorized.erase(clientfd);   //未就绪集合中移除
                fdToname[clientfd]=name;
                nameTofd[name]=clientfd;
            }
            else
            {//用户名在数据库中
                strcpy(sendmsg.src,"server");
                strcpy(sendmsg.buf,"该用户已经注册过，请登录");
                sendmsg.type=0;
                send(clientfd,&sendmsg,sizeof sendmsg,0);
            }
        }
        else if(recvmsg.type==1)
        {
            string ().swap(name);
            string ().swap(password);
            i=0;
            while(recvmsg.src[i]!='\0') //保存用户名
                name.push_back(recvmsg.src[i++]);

            i=0;
            while(recvmsg.buf[i]!='\0') //保存密码
                password.push_back(recvmsg.buf[i++]);

            query+=name;
            query+="\";";

            resultSet=stmt->executeQuery(query);    //改为你的库和表
            col_count=resultSet->getMetaData()->getColumnCount();
            //用户名相同
            if(resultSet->next())
            {
                string tmppassword=resultSet->getString(1).asStdString();
                if(tmppassword==password)
                {
                    //验证密码相同
                    strcpy(sendmsg.src,"server");
                    strcpy(sendmsg.buf,"OK");   //发送OK指令
                    sendmsg.type=1;
                    send(clientfd,&sendmsg,sizeof sendmsg,0);
                    client_list.insert(clientfd);   //插入就绪集合
                    unAuthorized.erase(clientfd);   //未就绪集合中移除
                    fdToname[clientfd]=name;
                    nameTofd[name]=clientfd;
                }
                else
                {
                    //密码不正确
                    strcpy(sendmsg.src,"server");
                    strcpy(sendmsg.buf,"密码不正确，请重新输入！");
                    sendmsg.type=1;
                    send(clientfd,&sendmsg,sizeof sendmsg,0);
                }
            }
            else
            {
                strcpy(sendmsg.src,"server");
                strcpy(sendmsg.buf,"用户名不存在，请重新输入！");
                sendmsg.type=1;
                send(clientfd,&sendmsg,sizeof sendmsg,0);
            }
        }
        else if(recvmsg.type==2)
        {
            sendmsg.type=2;
            if(client_list.size()==1)
            {//客户端列表中仅有一人，发送警告
                strcpy(sendmsg.src,"server");
                sprintf(sendmsg.buf,CAUTION);
                send(clientfd,&sendmsg,sizeof sendmsg,0);
                conn->close();
                delete stmt;
                delete conn;
                return len;
            }
            strcpy(sendmsg.src,recvmsg.src);

            strcpy(sendmsg.buf,recvmsg.src);
            strcpy(sendmsg.buf+strlen(sendmsg.buf)," say >> ");
            strcpy(sendmsg.buf+strlen(sendmsg.buf),recvmsg.buf);

            printf("%s\n",sendmsg.buf);
            //遍历客户端列表，群发消息
            for(unordered_set<int>:: iterator it=client_list.begin();it!=client_list.end();it++)
            {
                if(*it!=clientfd)
                {
                    if(send(*it,&sendmsg,sizeof sendmsg,0)<0)
                    {
                        perror("error");
                        exit(-1);
                    }
                }
            }
        }
        else if(recvmsg.type==3)
        {
            string ().swap(name);
            string ().swap(password);
            sendmsg.type=3;
            i=0;
            while(recvmsg.dst[i]!='\0') //保存接收者用户名
                name.push_back(recvmsg.dst[i++]);
            name.pop_back();            //将末尾的换行符

            if(!nameTofd.count(name))   //没有找到对应的描述符
            {//私聊用户不在线,通知发送端
                strcpy(sendmsg.src,"server");
                strcpy(sendmsg.dst,recvmsg.src);
                strcpy(sendmsg.buf,"该用户不在线上!");
                send(clientfd,&sendmsg,sizeof sendmsg,0);
                conn->close();
                delete stmt;
                delete conn;
                return len;
            }
            //将消息转发给接受端
            strcpy(sendmsg.src,recvmsg.src);
            strcpy(sendmsg.dst,recvmsg.dst);
            //将消息写入到message中
            strcpy(sendmsg.buf,recvmsg.src);
            strcpy(sendmsg.buf+strlen(sendmsg.buf)," say to me>> ");
            strcpy(sendmsg.buf+strlen(sendmsg.buf),recvmsg.buf);

            //遍历客户端列表，群发消息
            ret=send(nameTofd[name],&sendmsg,sizeof sendmsg,0);

            if(ret>0)
            {
                Msg secmsg;
                strcpy(secmsg.src,"server");
                strcpy(secmsg.dst,recvmsg.src);
                strcpy(secmsg.buf,"已经发送成功!\n");
                send(clientfd,&secmsg,sizeof secmsg,0);
            }
            else
            {
                Msg secmsg;
                strcpy(secmsg.src,"server");
                strcpy(secmsg.dst,recvmsg.src);
                strcpy(secmsg.buf,"发送失败!\n");
                send(clientfd,&secmsg,sizeof secmsg,0);
            }
        }
        else if(recvmsg.type==4)
        {
            sendmsg.type=4;
            strcpy(sendmsg.src,"server");
            strcpy(sendmsg.dst,recvmsg.src);
            char buff[1024];
            i=0;
            for(auto it=client_list.begin();it!=client_list.end();it++)
            {
                sprintf(buff,"%d. %s\n",i++,fdToname[*it].c_str());
                strcat(sendmsg.buf,buff);
            }
            send(clientfd,&sendmsg,sizeof sendmsg,0);
        }
        else
            printf("你输入的信息有误！\n");
    }

    conn->close();
    delete stmt;
    delete conn;
    //delete driver;

    return len;
}
#endif


