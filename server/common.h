#include<iostream>
#include<string.h>
using namespace std;


struct Msg
{
    Msg()
    {
        clear();
    }
    void clear()
    {
        bzero(src,20);
        bzero(dst,20);
        bzero(buf,1024);
    }
    char src[20];       //消息来源
    char dst[20];       //消息目的地
    char buf[1024];     //消息内容
    int type;           //消息类型
    //0.注册消息
    //1.登录消息
    //2.群发消息
    //3.私聊消息
    //4.在线用户信息
};

