#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "structs.h"

typedef void (*task_func)(void *);

typedef struct pool_task {
    task_func func;
    void *arg;
    struct pool_task *next;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int done;
} PoolTask;

typedef struct thread_pool {
    pthread_t *threads;
    int num_threads;
    PoolTask *head;
    PoolTask *tail;
    int stop;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_cond;
} ThreadPool;

void pool_init(ThreadPool *pool, int num_threads);
void pool_add_task(ThreadPool *pool, PoolTask *task);
void pool_wait_task(PoolTask *task);
void pool_destroy(ThreadPool *pool);

#endif // THREAD_POOL_H
