
#ifndef _BSAL_WORKER_POOL_H
#define _BSAL_WORKER_POOL_H

#include "worker.h"

struct bsal_node;
struct bsal_worker;

struct bsal_worker_pool {
    struct bsal_worker *worker_array;
    struct bsal_node *node;

    int worker_for_work;
    int worker_for_message;
    int worker_for_run;

    int workers;

    int ticks_without_messages;
};

void bsal_worker_pool_init(struct bsal_worker_pool *pool, int workers, struct bsal_node *node);
void bsal_worker_pool_destroy(struct bsal_worker_pool *pool);

void bsal_worker_pool_create_workers(struct bsal_worker_pool *pool);
void bsal_worker_pool_delete_workers(struct bsal_worker_pool *pool);
void bsal_worker_pool_run(struct bsal_worker_pool *pool);

void bsal_worker_pool_start(struct bsal_worker_pool *pool);
void bsal_worker_pool_stop(struct bsal_worker_pool *pool);

int bsal_worker_pool_pull(struct bsal_worker_pool *pool, struct bsal_message *message);

struct bsal_worker *bsal_worker_pool_select_worker_for_run(struct bsal_worker_pool *pool);
struct bsal_worker *bsal_worker_pool_select_worker_for_work(
                struct bsal_worker_pool *node, struct bsal_work *work);
struct bsal_worker *bsal_worker_pool_select_worker_for_message(struct bsal_worker_pool *pool);
int bsal_worker_pool_next_worker(struct bsal_worker_pool *node, int thread);

void bsal_worker_pool_schedule_work(struct bsal_worker_pool *pool, struct bsal_work *work);

int bsal_worker_pool_workers(struct bsal_worker_pool *pool);
int bsal_worker_pool_has_messages(struct bsal_worker_pool *pool);

#endif
