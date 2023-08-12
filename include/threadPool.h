#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

//任务结构体
typedef struct _PoolTask
{
    int tasknum;//模拟任务编号
    void *arg;//回调函数参数
    void (*task_func)(void *arg);//任务的回调函数
}PoolTask ;

//线程池结构体
typedef struct _ThreadPool
{
    int max_job_num;//最大任务个数
    int job_num;//实际任务个数

    u_int32_t beginnum;

    PoolTask *tasks;//任务队列数组
    int job_push;//入队位置
    int job_pop;// 出队位置

    int thr_num;//线程池内线程个数
    pthread_t *threads;//线程池内线程数组

    int shutdown;//是否关闭线程池

    pthread_mutex_t pool_lock;//线程池的锁
    pthread_cond_t empty_task;//任务队列为空的条件
    pthread_cond_t not_empty_task;//任务队列不为空的条件

}ThreadPool;

void create_threadpool(ThreadPool**pthreadPool, int thrnum,int maxtasknum);//创建线程池--thrnum  代表线程个数，maxtasknum 最大任务个数
void destroy_threadpool(ThreadPool *pool);//摧毁线程池
void addtask(ThreadPool *pool, void (*callback)(void *arg), void *arg);//添加任务到线程池
void *thrRun(void *arg);//线程执行的函数

#endif