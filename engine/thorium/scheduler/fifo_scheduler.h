
#ifndef THORIUM_FIFO_SCHEDULER_H
#define THORIUM_FIFO_SCHEDULER_H

#include <core/structures/fast_queue.h>

#include <stdint.h>

#define THORIUM_FIFO_SCHEDULER 1234

struct thorium_scheduler;
struct thorium_actor;

#define THORIUM_SCHEDULING_QUEUE_RATIO 64

/*
 * This is an implementation of a FIFO scheduler
 * for actors.
 */
struct thorium_fifo_scheduler {

    uint64_t max_priority_dequeue_operations;
    uint64_t high_priority_dequeue_operations;
    uint64_t normal_priority_dequeue_operations;
    uint64_t low_priority_dequeue_operations;

    struct biosal_fast_queue max_priority_queue;
    struct biosal_fast_queue high_priority_queue;
    struct biosal_fast_queue normal_priority_queue;
    struct biosal_fast_queue low_priority_queue;
};

extern struct thorium_scheduler_interface thorium_fifo_scheduler_implementation;

void thorium_fifo_scheduler_init(struct thorium_scheduler *self);
void thorium_fifo_scheduler_destroy(struct thorium_scheduler *self);

int thorium_fifo_scheduler_enqueue(struct thorium_scheduler *self, struct thorium_actor *actor);
int thorium_fifo_scheduler_dequeue(struct thorium_scheduler *self, struct thorium_actor **actor);

int thorium_fifo_scheduler_size(struct thorium_scheduler *self);

int thorium_fifo_scheduler_get_size_with_priority(struct thorium_fifo_scheduler *self, int priority);

struct biosal_fast_queue *thorium_fifo_scheduler_select_queue(struct thorium_fifo_scheduler *self, int priority);
uint64_t *thorium_fifo_scheduler_select_counter(struct thorium_fifo_scheduler *self, int priority);

int thorium_fifo_scheduler_dequeue_with_priority(struct thorium_fifo_scheduler *self, int priority,
                struct thorium_actor **actor);

void thorium_fifo_scheduler_reset_counter(struct thorium_fifo_scheduler *self, int priority);
uint64_t thorium_fifo_scheduler_get_counter(struct thorium_fifo_scheduler *self, int priority);

void thorium_fifo_scheduler_print(struct thorium_scheduler *self);
void thorium_fifo_scheduler_print_with_priority(struct thorium_fifo_scheduler *self, int priority, const char *name,
                int node, int worker);

#endif
