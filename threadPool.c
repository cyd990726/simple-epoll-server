#include "threadPool.h"


void *thrRun(void *arg)//线程执行的函数
{   
    printf("begin call %s-----\n", __FUNCTION__);
    ThreadPool *threadPoll = (ThreadPool *)arg;
    PoolTask *tmpTask = (PoolTask *)malloc(sizeof(PoolTask));
    while(1){
        //从队列取出任务
        pthread_mutex_lock(&threadPoll->pool_lock);
        while(threadPoll->job_num <= 0 && !threadPoll->shutdown){
            pthread_cond_wait(&threadPoll->not_empty_task, &threadPoll->pool_lock);
        }
        //若是销毁线程池，则退出线程
        if(threadPoll->shutdown){
            pthread_mutex_unlock(&threadPoll->pool_lock);
            free(tmpTask);
            pthread_exit(NULL);
        }
        memcpy(tmpTask, &threadPoll->tasks[threadPoll->job_pop], sizeof(PoolTask));

        threadPoll->job_pop = (threadPoll->job_pop+1)%threadPoll->max_job_num;
        threadPoll->job_num--;
        pthread_mutex_unlock(&threadPoll->pool_lock);
        pthread_cond_signal(&threadPoll->empty_task);
        //执行任务
        tmpTask->task_func(tmpTask->arg);
    }

}


void create_threadpool(ThreadPool**pthreadPool, int thrnum, int maxtasknum)//创建线程池--thrnum  代表线程个数，maxtasknum 最大任务个数
{
    printf("begin call %s-----\n",__FUNCTION__);
    //创建线程池
    *pthreadPool = (ThreadPool *)malloc(sizeof(ThreadPool));
    ThreadPool *threadPoll = *pthreadPool;

    //对线程池中的变量进行初始化
    threadPoll->max_job_num = maxtasknum;
    threadPoll->job_num = 0;
    threadPoll->tasks = (PoolTask *)malloc(sizeof(PoolTask) * maxtasknum);
    threadPoll->job_pop = 0;
    threadPoll->job_push = 0;
    threadPoll->thr_num = 0;
    threadPoll->shutdown = 0;
    threadPoll->beginnum = 0;

    pthread_mutex_init(&threadPoll->pool_lock, NULL);
    pthread_cond_init(&threadPoll->empty_task, NULL);
    pthread_cond_init(&threadPoll->not_empty_task, NULL);

    threadPoll->threads = (pthread_t *)malloc(sizeof(pthread_t) * thrnum);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int i = 0;
    for(; i<thrnum; i++){
        pthread_create(threadPoll->threads+i, &attr, &thrRun, threadPoll);
    }
    printf("线程池创建成功,线程数目%d,最大任务数目%d\n", thrnum, maxtasknum);

}
void destroy_threadpool(ThreadPool *pool)//摧毁线程池
{
    printf("begin call %s-----\n",__FUNCTION__);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->not_empty_task);
    //确保所有的线程都结束了，才释放资源。
    int i = 0;
    for(i = 0; i < pool->thr_num ; i++)
	{
        pthread_join(pool->threads[i],NULL);
    }
    pthread_mutex_destroy(&pool->pool_lock);
    pthread_cond_destroy(&pool->empty_task);
    pthread_cond_destroy(&pool->not_empty_task);

    free(pool->tasks);
    free(pool->threads);
    free(pool);
    printf("线程池销毁成功\n");
}

void addtask(ThreadPool *pool, void (*callback)(void *arg), void *arg)//添加任务到线程池
{
    printf("begin call %s-----%ld\n",__FUNCTION__, pthread_self());
    //添加任务
    pthread_mutex_lock(&pool->pool_lock);
    while(pool->job_num >= pool->max_job_num && !pool->shutdown){
        pthread_cond_wait(&pool->empty_task, &pool->pool_lock);
    }
    //若是销毁线程池了，就直接退出
    if(pool->shutdown){
        pthread_mutex_unlock(&pool->pool_lock);
        pthread_exit(NULL);
    }
    //入队
    pool->tasks[pool->job_push].tasknum = pool->beginnum++;
    pool->tasks[pool->job_push].arg = arg;
    pool->tasks[pool->job_push].task_func = callback;
    pool->job_push = (pool->job_push+1)%pool->max_job_num;
    pool->job_num++;
    pthread_mutex_unlock(&pool->pool_lock);
    pthread_cond_signal(&pool->not_empty_task);
    printf("end call %s-----%ld\n",__FUNCTION__, pthread_self());
}

