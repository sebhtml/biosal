
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

    struct core_fast_queue max_priority_queue;
    struct core_fast_queue high_priority_queue;
    struct core_fast_queue normal_priority_queue;
    struct core_fast_queue low_priority_queue;
};

extern struct thorium_scheduler_interface thorium_fifo_scheduler_implementation;

#endif
