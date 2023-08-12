#include "epollLoop.h"
#include "threadPool.h"
void acceptClient(void *arg){
    Server *server = ((Arg *)arg)->server;
    Xevent *xe = ((Arg *)arg)->xe;

    struct sockaddr_in addr;
    socklen_t len;
    int cfd = Accept(xe->fd, (struct sockaddr *)&addr, &len);
    char ip[16];
    inet_ntop(AF_INET, &addr.sin_addr, ip, 16);
    int port = ntohs(addr.sin_port);
    addEvent(server, cfd, EPOLLIN, ip, port, &readData);
}

void readData(void *arg){
    Server *server = ((Arg *)arg)->server;
    Xevent *xe = ((Arg *)arg)->xe;

    xe->buflen = recv(xe->fd, xe->buf, sizeof(xe->buf), 0);
    if(xe->buflen == 0){
        printf("client %s:%d closed\n", xe->ip, xe->port);
        //close(xe->fd);
        delEvent(server, xe->fd);
        return ;
    }
    if(xe->buflen < 0){
        printf("read data from client %s:%d error\n", xe->ip, xe->port);
       // close(xe->fd);
        delEvent(server, xe->fd);
        return ; 
    }
    printf("read %d bytes data from client %s:%d sucess\n",xe->buflen, xe->ip, xe->port);
    setEvent(server, xe->fd, EPOLLOUT, &writeData);
}

void writeData(void *arg){
    Server *server = ((Arg *)arg)->server;
    Xevent *xe = ((Arg *)arg)->xe;

    int ret = send(xe->fd, xe->buf, xe->buflen, 0);
    if(ret <= 0){
        printf("write data to client %s:%d sucess\n", xe->ip, xe->port);
      //  close(xe->fd);
        delEvent(server, xe->fd);
        return ;
    }
    printf("write %d bytes data to client %s:%d sucess\n", xe->buflen, xe->ip, xe->port);
    setEvent(server, xe->fd, EPOLLIN, &readData);
}

void addEvent(Server *server, int fd, int event, char *ip, uint16_t port, void (*callback)(void *arg)){
    //找出一块空的
    int i = 0;
    for(; i<server->maxevnets; i++){
        if(server->xevents[i].status == 0){
            break;
        }
    }
    if(i == server->maxevnets){
        printf("add event fail, not found the free node in %s\n", __FUNCTION__);
        return;
    }
    server->xevents[i].status = 1;
    server->xevents[i].fd = fd;
    server->xevents[i].event = event;
    server->xevents[i].lastactivetime = time(NULL);
    server->xevents[i].callback = callback;

    Arg *arg = (Arg *)malloc(sizeof(Arg));
    arg->server = server;
    arg->xe = &server->xevents[i];
    server->xevents[i].arg = arg;

    strcpy(server->xevents[i].ip, ip);

    server->xevents[i].port = port;

    struct epoll_event ev;
    ev.data.ptr = &server->xevents[i];
    ev.events = event | EPOLLET;
    //上树
   // printf("%d %d %d\n",server->epfd, fd , i);
    if(epoll_ctl(server->epfd, EPOLL_CTL_ADD, fd, &ev) < 0){
        //上树失败。
        server->xevents[i].status = 0;
        close(server->xevents[i].fd);
        printf("client %s:%d add falied\n", server->xevents[i].ip, server->xevents[i].port);
    }else{
        if(fd != server->lfd)
            server->cur_users++;
        printf("client %s:%d add sucess, cur_users:%d\n", server->xevents[i].ip, server->xevents[i].port, server->cur_users);
    }  
}

//修改树上监听的事件
void setEvent(Server *server, int fd, int event, void (*callback)(void *arg)){
    int i = 0;
    for(; i<server->maxevnets; i++){
        if(server->xevents[i].fd == fd && server->xevents[i].status){
            break;
        }
    }
    if(i == server->maxevnets){
        printf("not found the target node in %s\n", __FUNCTION__);
        return;
    }
    server->xevents[i].event = event;
    server->xevents[i].callback = callback;
    server->xevents[i].lastactivetime = time(NULL);//更新一下时间

    struct epoll_event ev;
    ev.data.ptr = &server->xevents[i];
    ev.events = event| EPOLLET;

    if(epoll_ctl(server->epfd, EPOLL_CTL_MOD, fd, &ev) < 0){
        printf("client %s:%d setEvent failed \n", server->xevents[i].ip, server->xevents[i].port);
    }else{
        printf("client %s:%d setEvent success \n", server->xevents[i].ip, server->xevents[i].port);
    }
}

void delEvent(Server *server, int fd){

    int i = 0;
    for(; i<server->maxevnets; i++){
        if(server->xevents[i].fd == fd && server->xevents[i].status){
            break;
        }
    }
    if(i == server->maxevnets){
        printf("not found the target node in %s\n", __FUNCTION__);
        return;
    }
    //下树
    //难怪刚才一直报错，原来要先下树再close，要是先close，下树就会报错。
    if(epoll_ctl(server->epfd, EPOLL_CTL_DEL, fd, NULL) < 0){
        printf("client %s:%d delEvent failed \n", server->xevents[i].ip, server->xevents[i].port);
    }else{
        server->xevents[i].status = 0;
        Close(fd);
        server->cur_users--;
        printf("client %s:%d delEvent sucess, cur_user:%d \n", server->xevents[i].ip, server->xevents[i].port, server->cur_users);
    }

}
void initServer(Server **pserver, char *ip, uint16_t port, int maxevents){
    *pserver = (Server *)malloc(sizeof(Server));
    Server *server = *pserver;

    //初始化服务器基础参数
    strncpy(server->ip, ip, 16);
    server->port = port;
    server->maxevnets = maxevents;
    server->xevents = (Xevent*)malloc(sizeof(Xevent) * maxevents);
    server->events = (struct epoll_event *)malloc(sizeof(struct epoll_event)*maxevents);
    bzero(server->xevents, sizeof(Xevent)*maxevents);
    bzero(server->events, sizeof(struct epoll_event)*maxevents);
    server->epfd = epoll_create(1);
    server->cur_users = 0;
    server->lfd = tcp4bind(server->port, server->ip);
    Listen(server->lfd, 128);

    //将lfd上数
    addEvent(server, server->lfd, EPOLLIN, ip, port, &acceptClient);
     
}

void serverRun(Server *server, ThreadPool *pool){
    long lastDetectTime = time(NULL), nowTime;
    int i = 0, checkPos = 0;//checkPos:循环检测的下标
    while(1){
        //每10秒检测将不活跃连接下树
        nowTime = time(NULL);
        if(nowTime-lastDetectTime >= 10){
            //开始检测
            lastDetectTime = nowTime;
            for(i = 0; i<100; i++, checkPos++){
                if(checkPos == server->maxevnets){
                    checkPos = 0;
                }
                //如果不在树上或者文件描述符是lfd， 就跳过
                if((!server->xevents[i].status) || server->xevents[i].fd == server->lfd){
                    continue;
                }
                //超过60秒没活跃的，都下树关闭连接
                long dur = nowTime - server->xevents[i].lastactivetime;
                if(dur >= 60){
                    //下树，关闭连接
                    printf("client %s:%d timeout, is closed\n", server->xevents[i].ip, server->xevents[i].port);
                 //   close(server->xevents[i].fd);
                    delEvent(server, server->xevents[i].fd);
                }

            }
        }


        int n = epoll_wait(server->epfd, server->events, server->maxevnets, -1);
        if(n <= 0){
            printf("epoll_wait error\n");
            break;
        }
        int i = 0;
        Xevent *xe = NULL;
        for(; i<n; i++){
            xe = (Xevent *)server->events[i].data.ptr;
            addtask(pool, xe->callback, xe->arg);
           // sleep(2);
            //xe->callback(xe->arg);
        }
    }
}
