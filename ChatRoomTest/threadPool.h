#ifndef __THREAD_POOL_H_
#define __THREAD_POOL_H_

#include <pthread.h>


/* 任务结点 */
typedef struct task_t
{
    void *(*worker_hander)(void *arg);
    void *arg;
} task_t;

/* 线程池 */
typedef struct threadPool_t
{
    /* 任务队列 -- 将之设计成循环队列 */
    struct task_t * taskQueue;
    /* 任务队列容量 */
    int queueCapacity;
    /* 任务队列任务数大小 */
    int queueSize;
    /* 任务队列的头 */
    int queueFront; 
    /* 任务队列的尾 */
    int queueRear;
    
    /* 工作线程ID */
    pthread_t *threadIds;
    /* 管理着线程 */
    pthread_t managerThread;

    /* 最小的线程数 */
    int minThreads;
    /* 最大的线程数 */
    int maxThreads;
    /* 忙碌的线程数 */
    int busyThreadNums;
    /* 存活的线程数 */
    int liveThreadNums;
    /* 销毁的线程数 */
    int exitThreadNums;

    /* 锁 - 锁住这个线程池内部的属性 */
    pthread_mutex_t mutexPool;
    /* 锁 - 锁住忙线程的属性 */
    pthread_mutex_t mutexBusy;

    /* 条件变量 - 消费者向生产者发送 目的: 可以继续生产 */
    pthread_cond_t notFull;
    /* 条件变量 - 生产者向消费者发送 目的: 可以继续消费 */
    pthread_cond_t notEmpty;

    /* 关闭 */
    int shoudown;
} threadPool_t;

/* 初始化线程池 */
int threadPoolInit(threadPool_t * pool, int minThreadNums, int maxThreadNums, int taskQueueCapacity);

/* 销毁线程池 */
int threadPoolDestroy(threadPool_t * pool);

/* 添加任务 */
int threadPoolAddTask(threadPool_t * pool, void *(*worker_hander)(void * arg), void *arg);

#endif //__THREAD_POOL_H_