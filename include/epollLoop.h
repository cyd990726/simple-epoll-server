#ifndef _EPOLLLOOP_H_
#define _EPOLLLOOP_H_

#include <sys/select.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include "wrap.h"
#include "threadPool.h"


typedef struct _Xevent{

    int fd;//文件描述符
    int event; //监听的事件
    char buf[1024];//缓冲区
    int buflen; //缓冲区尺寸

    char ip[16];//此文件描述符的ip和端口
    uint16_t port;

    void *arg; //回调函数的参数
    void (*callback)(void *arg);//回调函数

    long lastactivetime;//最后活跃时间
    int status;//标志是否在树上

}Xevent;

typedef struct _Server{
    int epfd;//事件数的根

    int lfd;
    char ip[16];//ip
    uint16_t port;//端口
    int maxevnets;//最大监听事件数目
    Xevent *xevents;//最大监听事件数组

    struct epoll_event *events;//存储监听事件发生的数组

    int cur_users;//当前用户数
}Server;

typedef struct _Arg{
    Server *server;
    Xevent *xe;
    
}Arg;

void acceptClient(void *arg);
void readData(void *arg);

void writeData(void *arg);
void addEvent(Server *server, int fd, int event, char *ip, uint16_t port, void (*callback)(void *arg));

//修改树上监听的事件
void setEvent(Server *server, int fd, int event, void (*callback)(void *arg));
void delEvent(Server *server, int fd);
void initServer(Server **pserver, char *ip, uint16_t port, int maxevents);

void serverRun(Server *server, ThreadPool *pool);

#endif