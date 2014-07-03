
#include "worker_pool.h"

#include <core/helpers/vector_helper.h>

#define BSAL_WORKER_POOL_MESSAGE_SCHEDULING_WINDOW 4

int bsal_worker_pool_next_worker(struct bsal_worker_pool *pool, int worker)
{
    worker++;

    /* wrap the counter
     */
    if (worker == pool->workers) {
        worker = 0;
    }

    return worker;
}

struct bsal_worker *bsal_worker_pool_get_worker(
                struct bsal_worker_pool *self, int index)
{
    return self->worker_cache + index;
}

void bsal_worker_pool_set_cached_value(struct bsal_worker_pool *self, int index, int value)
{
    self->message_cache[index] = value;
}

int bsal_worker_pool_get_cached_value(struct bsal_worker_pool *self, int index)
{
    return self->message_cache[index];
}

/* select a worker to pull from */
struct bsal_worker *bsal_worker_pool_select_worker_for_message(struct bsal_worker_pool *pool)
{
    int index;
    int i;
    int score;
    struct bsal_worker *worker;
    int attempts;
    struct bsal_worker *best_worker;
    int best_score;
    int best_index;

    best_index = -1;
    best_score = 0;
    best_worker = NULL;

    i = 0;
    attempts = BSAL_WORKER_POOL_MESSAGE_SCHEDULING_WINDOW;

    /* select thet worker with the most messages in the window.
     */
    while (i < attempts) {

        index = pool->worker_for_message;
        pool->worker_for_message = bsal_worker_pool_next_worker(pool, index);
        worker = bsal_worker_pool_get_worker(pool, index);
        score = bsal_worker_pool_get_cached_value(pool, index);

        /* Update the cache.
         * This is expensive because it will touch the cache line.
         * Only the worker is increasing the number of messages, and
         * only the worker pool is decreasing it.
         * As long as the cached value is greater than 0, then there is
         * definitely something to pull without the need
         * to break the CPU cache line
         *
         */
        /* always update cache because otherwise there will be
         * starvation
         */
        if (1 || score == 0) {
            score = bsal_worker_get_message_production_score(worker);
            bsal_worker_pool_set_cached_value(pool, index, score);
        }

        if (best_worker == NULL || score > best_score) {
            best_worker = worker;
            best_score = score;
            best_index = index;
        }

        ++i;
    }

    /* Update the cached value for the winning worker to have an
     * accurate value for this worker.
     */
    bsal_vector_helper_set_int(&pool->message_count_cache, best_index, best_score - 1);

    return best_worker;
}

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
int bsal_worker_pool_pull_classic(struct bsal_worker_pool *pool, struct bsal_message *message)
{
    struct bsal_worker *worker;
    int answer;

    worker = bsal_worker_pool_select_worker_for_message(pool);
    answer = bsal_worker_pull_message(worker, message);

    return answer;
}
#endif

int bsal_worker_pool_pull(struct bsal_worker_pool *pool, struct bsal_message *message)
{
    int answer;

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
    answer = bsal_worker_pool_pull_classic(pool, message);
#else
    answer = bsal_message_queue_dequeue(&pool->message_queue, message);
#endif

#if 0
    if (!answer) {
        pool->ticks_without_messages++;
    } else {
        pool->ticks_without_messages = 0;
    }
#endif

    return answer;
}


