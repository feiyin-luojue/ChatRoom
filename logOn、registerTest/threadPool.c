#include "threadPool.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define NUMBER  2
/* 状态码 */
enum STATUS_CODE
{
    ON_SUCCESS,
    NULL_PTR,
    MALLOC_ERROR,
    INVALID_ACCESS,
    THREAD_CREATE_ERR,
    UNKNOWN_ERROR,
};

#define DEFAULT_MIN_THREADS 5
#define DEFAULT_MAX_THREADS 100
#define DEFAULT_MAX_QUEUES 100





/* 静态函数前置声明 */
static void * thread_Hander(void *arg);
static void * manager_Hander(void *arg);

static int pthreadExitClearThreadIndex(threadPool_t *pool)
{
    int ret = 0;
    
    /* 需要判断当前线程是在数组threads对应的哪一个索引里面. */
    for (int idx = 0; idx < pool->maxThreads; idx++)
    {
        //printf("threadId[%d]: %ld -> pthread_self: %ld\n", idx, pool->threadIds[idx], pthread_self());
        if (pool->threadIds[idx] == pthread_self())
        {
            /* 将需要退出的线程 对应的索引清空. */
            pool->threadIds[idx] = 0;
            printf("我清理好了！下班了\n");
            break;
        }
    }
    
    pthread_exit(NULL);
    return ret;
}
/* 本质是一个消费者函数 */
static void * thread_Hander(void *arg)
{
    //pthread_detach(pthread_self());
    //printf("我出生了！:%ld\n", pthread_self());
    threadPool_t *pool = (threadPool_t * )arg;
    sleep(2);//???
    while (1)
    {
        pthread_mutex_lock(&(pool->mutexPool));
        /* 没有任务消费的时候 */
        while (pool->queueSize == 0 && pool->shoudown == 0)
        {
            pthread_cond_wait(&(pool->notEmpty), &(pool->mutexPool));

            if (pool->exitThreadNums > 0)
            {
                /* 需要销毁的线程数减一. */
                pool->exitThreadNums--;

                if (pool->liveThreadNums > pool->minThreads)
                {
                    /* 活着的线程数减一 */
                    pool->liveThreadNums--;
                    /* 解锁 */
                    pthread_mutex_unlock(&(pool->mutexPool));
                    pthreadExitClearThreadIndex(pool);
                    // pthread_exit(NULL);
                }
            
            }
        }

        /* 销毁线程池 标识位置1. */
        if (pool->shoudown == 1)
        {
            /* 解锁 */
            pool->liveThreadNums--;
            pthread_mutex_unlock(&(pool->mutexPool));
            printf("我去收拾东西下班了！\n");
            /* 线程退出 */
            pthreadExitClearThreadIndex(pool);
        }

        /* 从任务队列中取数据 -- 从队列的对头取数据 */
        task_t task = pool->taskQueue[pool->queueFront];
        /* 任务队列的任务数减一. */
        pool->queueSize--;
        /* front 向后移动. */
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pthread_mutex_unlock(&(pool->mutexPool));
        pthread_cond_signal(&(pool->notFull));

        
        pthread_mutex_lock(&(pool->mutexBusy));
        /* 忙碌的线程数加一. */
        pool->busyThreadNums++;
        pthread_mutex_unlock(&(pool->mutexBusy));

        /* 执行处理函数 */
        printf("thread %ld start working...\n", pthread_self());
        task.worker_hander(task.arg);
        printf("thread %ld end working...\n", pthread_self());
        
        //sleep(10);
        pthread_mutex_lock(&(pool->mutexBusy));
        //printf("busyNum:%d liveNum:%d\n", pool->busyThreadNums, pool->liveThreadNums);
        //printf("thread %ld end working...\n", pthread_self());
        /* 忙碌的线程数加一. */
        pool->busyThreadNums--;
        pthread_mutex_unlock(&(pool->mutexBusy));
    }

}

static void * manager_Hander(void *arg)
{
    threadPool_t *pool = (threadPool_t * )arg;
    while (pool->shoudown != 1)
    {
        sleep(3);
        pthread_mutex_lock(&(pool->mutexPool));
        int liveThreadNums = pool->liveThreadNums;
        int queueSize = pool->queueSize;
        pthread_mutex_unlock(&(pool->mutexPool));

        pthread_mutex_lock(&(pool->mutexBusy));
        int busyThreadNums = pool->busyThreadNums;
        pthread_mutex_unlock(&(pool->mutexBusy));

        /* 扩大线程池中线程的容量 */
        /* 任务的个数 > 存活的线程个数 && 存活的线程数 < 最大线程数 */
        if (queueSize > liveThreadNums && liveThreadNums < pool->maxThreads)
        {   
            int count = 0;
            /* 加锁 */
            pthread_mutex_lock(&(pool->mutexPool));
            for (int idx = 0; idx < pool->maxThreads && count < NUMBER
                    && pool->liveThreadNums < pool->maxThreads; idx++)
            {
                if (pool->threadIds[idx] == 0)
                {
                    pthread_create(&(pool->threadIds[idx]), NULL, thread_Hander, pool);
                    count++;
                    /* 更新线程池存活的线程数 */
                    pool->liveThreadNums++;
                }
            }
            /* 解锁 */
            pthread_mutex_unlock(&(pool->mutexPool));
        }

        /* 缩小线程池中线程的容量 */
        /* 忙的线程数 * 2 < 存活的线程数 && 存活的线程 > 最小线程数 */
        if (busyThreadNums * 2 < liveThreadNums && liveThreadNums > pool->minThreads)
        {
            pthread_mutex_lock(&(pool->mutexPool));
            pool->exitThreadNums = NUMBER;
            pthread_mutex_unlock(&(pool->mutexPool));

            /* 唤醒等待工作的线程 -- 等待工作的线程就是空闲的线程 */
            for (int idx = 0; idx < NUMBER; idx++)
            {
                /* 发送信号. */
                pthread_cond_signal(&(pool->notEmpty));
            }
        }
    }
    pthread_exit(NULL);
}


/* 初始化线程池 */
int threadPoolInit(threadPool_t * pool, int minThreadNums, int maxThreadNums, int taskQueueCapacity)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }

    /* do ... while 循环 */
    do 
    {
        /* 判断合法性 */
        if (minThreadNums <= 0 || maxThreadNums <= 0 || minThreadNums > maxThreadNums)
        {
            minThreadNums = DEFAULT_MIN_THREADS;
            maxThreadNums = DEFAULT_MAX_THREADS;   
        }
        
        /* 判断合法性 */
        if (taskQueueCapacity < 0) 
        {
            taskQueueCapacity = DEFAULT_MAX_QUEUES;
        }

        pool->minThreads = minThreadNums;
        pool->maxThreads = maxThreadNums;
        /* 队列的容量 */
        pool->queueCapacity = taskQueueCapacity;
        /* 队列任务数大小 */
        pool->queueSize = 0;

        /* 任务队列 */
        pool->taskQueue = (struct task_t *)malloc(sizeof(struct task_t) * (pool->queueCapacity));
        if (pool->taskQueue == NULL)
        {
            perror("malloc erorr");
            break;
        }
        /* 循环队列队头位置 */
        pool->queueFront = 0;
        /* 循环队列队尾位置 */
        pool->queueRear = 0;

        pool->threadIds = (pthread_t *)malloc(sizeof(pthread_t) * pool->maxThreads);
        //printf("threadIds:%p\n", pool->threadIds);
        if (pool->threadIds == NULL)
        {
            perror("malloc error");
            break;
        }
        /* 清除脏数据 */
        memset(pool->threadIds, 0, sizeof(pthread_t) * pool->maxThreads);

        int ret = 0;
        /* 工作线程的创建 */
        for (int idx = 0; idx < pool->minThreads; idx++)
        {   
            ret = pthread_create(&(pool->threadIds[idx]), NULL, thread_Hander, (void *)pool);
            //printf("创建了一个线程->%ld\n", pool->threadIds[idx]);
            if (ret != 0)
            {
                perror("pthread create error");
                break;
            }
        }

        // printf("checking!\n");
        // for(int jdx = 0; jdx < pool->maxThreads; jdx++)
        // {
        //     printf("thread[%d]: %ld\n", jdx, pool->threadIds[jdx]);
        // }

        /* 管理者线程的创建 */
        ret = pthread_create(&pool->managerThread, NULL, manager_Hander, (void *)pool);
        if (ret != 0)
        {
            perror("pthread create error");
            break;
        }
        pool->liveThreadNums = minThreadNums;
        pool->busyThreadNums = 0;
        

        if (
            pthread_mutex_init(&(pool->mutexPool), NULL) != 0 || 
            pthread_mutex_init(&(pool->mutexBusy), NULL) != 0
            )
        {
            perror("mutex error ");
            break;
        }

        if (
            pthread_cond_init(&(pool->notFull), NULL) != 0    || 
            pthread_cond_init(&(pool->notEmpty), NULL) != 0
        )
        {
            perror("cond error ");
            break;
        }

        /* 销毁线程池 标识位置0. */
        pool->shoudown = 0;
        return ON_SUCCESS;
    }while (0);
    
    /* 程序到达这边意味着上面初始化流程出现了错误 */

    /* 释放内存 */
    if (pool->taskQueue != NULL)
    {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }

    if (pool->threadIds)
    {
        /* 阻塞回收工作线程资源 */
        for (int idx = 0; idx < pool->minThreads; idx++)
        {
            if (pool->threadIds[idx] != 0)
            {
                pthread_join(pool->threadIds[idx], NULL);
            }
        }

        /* 释放内存 */
        free(pool->threadIds);
        pool->threadIds = NULL;
    }

    /* 阻塞回收管理者线程资源 */
    if (pool->managerThread != 0)
    {
        pthread_join(pool->managerThread, NULL);
    }

    /* 释放锁和条件变量 */
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_cond_destroy(&(pool->notEmpty));
    pthread_cond_destroy(&(pool->notFull));

    return UNKNOWN_ERROR;
}

/* 添加任务 */
/* 本质上是生产者 */
int threadPoolAddTask(threadPool_t * pool, void *(*worker_hander)(void * arg), void *arg)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }

    /* 加锁 */
    pthread_mutex_lock(&(pool->mutexPool));
    /* 任务队列满了 */
    while (pool->queueSize == pool->queueCapacity && pool->shoudown == 0)
    {
        /* 等待条件变量 : 不满的条件变量 */
        pthread_cond_wait(&(pool->notFull), &(pool->mutexPool));
    }

    if (pool->shoudown != 0)
    {
        pthread_mutex_unlock(&(pool->mutexPool));
        return ON_SUCCESS;
    }

    /* 将新的任务 添加到任务队列中 */
    pool->taskQueue[pool->queueRear].worker_hander = worker_hander;
    pool->taskQueue[pool->queueRear].arg = arg;
    /* 任务个数加一 */
    pool->queueSize++;
    /* 该队列是循环队列 -- 要让此索引循环起来. */
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;

    pthread_mutex_unlock(&(pool->mutexPool));
    pthread_cond_signal(&(pool->notEmpty));

    return ON_SUCCESS;
}


int threadPoolDestroy(threadPool_t * pool)
{
    if (pool == NULL)
    {
        return NULL_PTR;
    }

    pool->shoudown = 1;
    printf("下班了！！！\n");

    /* 阻塞回收管理者线程 */
    pthread_join(pool->managerThread, NULL);

    /* 唤醒阻塞的消费者线程 */
    for (int idx = 0; idx < pool->liveThreadNums; idx++)
    {
        pthread_cond_signal(&(pool->notEmpty));
    }

    sleep(3);
    /* 释放堆空间 */
    if (pool->taskQueue)
    {
        free(pool->taskQueue);
        pool->taskQueue = NULL;
    }
    

    if (pool->threadIds)
    {
        free(pool->threadIds);
        pool->threadIds = NULL;
    }

    pthread_mutex_destroy(&(pool->mutexPool));
    pthread_mutex_destroy(&(pool->mutexBusy));
    pthread_cond_destroy(&(pool->notEmpty));
    pthread_cond_destroy(&(pool->notFull));

    // if (pool)
    // {
    //     free(pool);
    //     pool = NULL;
    // }

    return ON_SUCCESS;
}