
#include "worker_pool.h"

#include "actor.h"
#include "work.h"
#include "worker_thread.h"

#include <stdlib.h>
#include <stdio.h>

void bsal_worker_pool_init(struct bsal_worker_pool *pool, int workers,
                struct bsal_node *node)
{
    pool->node = node;
    pool->workers = workers;
    pool->worker_array = NULL;

    /* with only one thread,  the main thread
     * handles everything.
     */
    if (pool->workers >= 1) {
        pool->worker_for_run = 0;
        pool->worker_for_message = 0;
        pool->worker_for_work = 0;
    } else {
        printf("Error: the number of workers must be at least 1.\n");
        exit(1);
    }

    bsal_worker_pool_create_workers(pool);
}

void bsal_worker_pool_destroy(struct bsal_worker_pool *pool)
{
    bsal_worker_pool_delete_workers(pool);
}

void bsal_worker_pool_delete_workers(struct bsal_worker_pool *pool)
{
    int i = 0;

    if (pool->workers <= 0) {
        return;
    }

    for (i = 0; i < pool->workers; i++) {
        bsal_worker_thread_destroy(pool->worker_array + i);
    }

    free(pool->worker_array);
    pool->worker_array = NULL;
}

void bsal_worker_pool_create_workers(struct bsal_worker_pool *pool)
{
    int bytes;
    int i;

    if (pool->workers <= 0) {
        return;
    }

    bytes = pool->workers * sizeof(struct bsal_worker_thread);
    pool->worker_array = (struct bsal_worker_thread *)malloc(bytes);

    for (i = 0; i < pool->workers; i++) {
        bsal_worker_thread_init(pool->worker_array + i, i, pool->node);
    }
}

void bsal_worker_pool_start(struct bsal_worker_pool *pool)
{
    int i;

    /* start workers
     *
     * we start at 1 because the thread 0 is
     * used by the main thread...
     */
    for (i = 0; i < pool->workers; i++) {
        bsal_worker_thread_start(pool->worker_array + i);
    }
}

void bsal_worker_pool_run(struct bsal_worker_pool *pool)
{
    /* make the thread work (this is the main thread) */
    bsal_worker_thread_run(bsal_worker_pool_select_worker_for_run(pool));
}

void bsal_worker_pool_stop(struct bsal_worker_pool *pool)
{
    int i;
    /*
     * stop workers
     */

    for (i = 0; i < pool->workers; i++) {
        bsal_worker_thread_stop(pool->worker_array + i);
    }
}

int bsal_worker_pool_pull(struct bsal_worker_pool *pool, struct bsal_message *message)
{
    struct bsal_worker_thread *thread;

    thread = bsal_worker_pool_select_worker_for_message(pool);
    return bsal_worker_thread_pull_message(thread, message);
}

/* select a thread to pull from */
struct bsal_worker_thread *bsal_worker_pool_select_worker_for_message(struct bsal_worker_pool *pool)
{
    int index;

    index = pool->worker_for_message;
    pool->worker_for_message = bsal_worker_pool_next_worker(pool, pool->worker_for_message);
    return pool->worker_array + index;
}

int bsal_worker_pool_next_worker(struct bsal_worker_pool *pool, int worker)
{
    worker++;
    worker %= pool->workers;

    return worker;
}

/* select the thread to push work to */
struct bsal_worker_thread *bsal_worker_pool_select_worker_worker_for_work(
                struct bsal_worker_pool *pool, struct bsal_work *work)
{
    int index;
    struct bsal_worker_thread *thread;

    /* check if actor has an affinity thread */
    thread = bsal_actor_affinity_thread(bsal_work_actor(work));

    if (thread != NULL) {
        return thread;
    }

    /* otherwise, pick a thread with round robin */
    index = pool->worker_for_message;
    pool->worker_for_message = bsal_worker_pool_next_worker(pool, pool->worker_for_message);
    return pool->worker_array + index;
}

struct bsal_worker_thread *bsal_worker_pool_select_worker_for_run(struct bsal_worker_pool *pool)
{
    int index;

    index = pool->worker_for_run;
    return pool->worker_array + index;
}

/*
 * names are based on names found in:
 * \see http://lxr.free-electrons.com/source/include/linux/workqueue.h
 * \see http://lxr.free-electrons.com/source/kernel/workqueue.c
 */
void bsal_worker_pool_schedule_work(struct bsal_worker_pool *pool, struct bsal_work *work)
{
    struct bsal_worker_thread *thread;

    thread = bsal_worker_pool_select_worker_worker_for_work(pool, work);

    /* bsal_worker_thread_push_message use a spinlock to spin fast ! */
    bsal_worker_thread_push_work(thread, work);
}

int bsal_worker_pool_workers(struct bsal_worker_pool *pool)
{
    return pool->workers;
}
