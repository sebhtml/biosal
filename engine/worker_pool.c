
#include "worker_pool.h"
#include "worker_thread.h"

#include <stdlib.h>
#include <stdio.h>

void bsal_worker_pool_init(struct bsal_worker_pool *pool, int threads,
                struct bsal_node *node)
{
    pool->node = node;
    pool->threads = threads;
    pool->thread_array = NULL;

    /* with only one thread,  the main thread
     * handles everything.
     */
    if (pool->threads == 1) {
        pool->thread_for_run = 0;
        pool->thread_for_message = 0;
        pool->thread_for_work = 0;
    } else if (pool->threads > 1) {
        pool->thread_for_run = 0;
        pool->thread_for_message = 1;
        pool->thread_for_work = 1;
    } else {
        printf("Error: the number of threads must be at least 1.\n");
        exit(1);
    }

    bsal_worker_pool_create_threads(pool);
}

void bsal_worker_pool_destroy(struct bsal_worker_pool *pool)
{
    bsal_worker_pool_delete_threads(pool);
}

void bsal_worker_pool_delete_threads(struct bsal_worker_pool *pool)
{
    int i = 0;

    if (pool->threads <= 0) {
        return;
    }

    for (i = 0; i < pool->threads; i++) {
        bsal_worker_thread_destroy(pool->thread_array + i);
    }

    free(pool->thread_array);
    pool->thread_array = NULL;
}

void bsal_worker_pool_create_threads(struct bsal_worker_pool *pool)
{
    int bytes;
    int i;

    if (pool->threads <= 0) {
        return;
    }

    bytes = pool->threads * sizeof(struct bsal_worker_thread);
    pool->thread_array = (struct bsal_worker_thread *)malloc(bytes);

    for (i = 0; i < pool->threads; i++) {
        bsal_worker_thread_init(pool->thread_array + i, i, pool->node);
    }
}

void bsal_worker_pool_start(struct bsal_worker_pool *pool)
{
    int i;

    /* start threads
     *
     * we start at 1 because the thread 0 is
     * used by the main thread...
     */
    for (i = 1; i < pool->threads; i++) {
        bsal_worker_thread_start(pool->thread_array + i);
    }
}

void bsal_worker_pool_run(struct bsal_worker_pool *pool)
{
    if (pool->threads > 1) {
        return;
    }

    /* make the thread work (this is the main thread) */
    bsal_worker_thread_run(bsal_worker_pool_select_worker_thread(pool));
}

void bsal_worker_pool_stop(struct bsal_worker_pool *pool)
{
    int i;
    /*
     * stop threads
     */

    for (i = 1; i < pool->threads; i++) {
        bsal_worker_thread_stop(pool->thread_array + i);
    }
}

int bsal_worker_pool_pull(struct bsal_worker_pool *pool, struct bsal_message *message)
{
    struct bsal_worker_thread *thread;

    thread = bsal_worker_pool_select_worker_thread_for_message(pool);
    return bsal_worker_thread_pull_message(thread, message);
}

/* select a thread to pull from */
struct bsal_worker_thread *bsal_worker_pool_select_worker_thread_for_message(struct bsal_worker_pool *pool)
{
    int index;

    index = pool->thread_for_message;
    pool->thread_for_message = bsal_worker_pool_next_worker(pool, pool->thread_for_message);
    return pool->thread_array + index;
}

int bsal_worker_pool_next_worker(struct bsal_worker_pool *pool, int thread)
{
    /*
     * 1 thread : thread 0 (main thread)
     * 2 threads : thread 0 (main thread) and thread 1 (worker)
     * 3 threads : thread 0 (main thread) and thread 1 (worker) and thread 2 (worker)
     * N threads (N >= 2) : thread 0
     */
    if (pool->threads >= 3) {
        thread++;
        if (thread == pool->threads) {
            thread = 1;
        }
    }

    return thread;
}

/* select the thread to push work to */
struct bsal_worker_thread *bsal_worker_pool_select_worker_thread_for_work(struct bsal_worker_pool *pool)
{
    int index;

    index = pool->thread_for_message;
    pool->thread_for_message = bsal_worker_pool_next_worker(pool, pool->thread_for_message);
    return pool->thread_array + index;
}

struct bsal_worker_thread *bsal_worker_pool_select_worker_thread(struct bsal_worker_pool *pool)
{
    int index;

    index = pool->thread_for_run;
    return pool->thread_array + index;
}

/*
 * \see http://lxr.free-electrons.com/source/include/linux/workqueue.h
 * \see http://lxr.free-electrons.com/source/include/linux/workqueue.c
 */
void bsal_worker_pool_schedule_work(struct bsal_worker_pool *pool, struct bsal_work *work)
{
    struct bsal_worker_thread *thread;

    thread = bsal_worker_pool_select_worker_thread_for_work(pool);

    /* bsal_worker_thread_push_message use a spinlock to spin fast ! */
    bsal_worker_thread_push_work(thread, work);
}
