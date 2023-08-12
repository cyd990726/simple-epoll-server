#include "epollLoop.h"
#include "threadPool.h"


int main(){
    ThreadPool *pool = NULL;
    create_threadpool(&pool, 3, 20);
    Server *server = NULL;
    initServer(&server, "127.0.0.1", 1234, 1024);
    serverRun(server, pool);
    return 0;
}