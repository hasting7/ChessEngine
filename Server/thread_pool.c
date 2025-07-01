#include "includes/chess.h"
#include "includes/thread_pool.h"
#include <stdlib.h>

static void *worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    while (1) {
        pthread_mutex_lock(&pool->queue_lock);
        while (!pool->head && !pool->stop) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_lock);
        }
        if (pool->stop && !pool->head) {
            pthread_mutex_unlock(&pool->queue_lock);
            break;
        }
        PoolTask *task = pool->head;
        pool->head = task->next;
        if (!pool->head)
            pool->tail = NULL;
        pthread_mutex_unlock(&pool->queue_lock);

        task->func(task->arg);

        pthread_mutex_lock(&task->lock);
        task->done = 1;
        pthread_cond_signal(&task->cond);
        pthread_mutex_unlock(&task->lock);
    }
    return NULL;
}

void pool_init(ThreadPool *pool, int num_threads) {
    pool->threads = malloc(sizeof(pthread_t) * num_threads);
    pool->num_threads = num_threads;
    pool->head = pool->tail = NULL;
    pool->stop = 0;
    pthread_mutex_init(&pool->queue_lock, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&pool->threads[i], NULL, worker, pool);
    }
}

void pool_add_task(ThreadPool *pool, PoolTask *task) {
    task->next = NULL;
    task->done = 0;
    pthread_mutex_init(&task->lock, NULL);
    pthread_cond_init(&task->cond, NULL);

    pthread_mutex_lock(&pool->queue_lock);
    if (pool->tail)
        pool->tail->next = task;
    else
        pool->head = task;
    pool->tail = task;
    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_lock);
}

void pool_wait_task(PoolTask *task) {
    pthread_mutex_lock(&task->lock);
    while (!task->done) {
        pthread_cond_wait(&task->cond, &task->lock);
    }
    pthread_mutex_unlock(&task->lock);
    pthread_mutex_destroy(&task->lock);
    pthread_cond_destroy(&task->cond);
}

void pool_destroy(ThreadPool *pool) {
    pthread_mutex_lock(&pool->queue_lock);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_lock);

    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_destroy(&pool->queue_lock);
    pthread_cond_destroy(&pool->queue_cond);
    free(pool->threads);
}
