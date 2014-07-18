
#ifndef BSAL_SCHEDULING_QUEUE
#define BSAL_SCHEDULING_QUEUE

#include <core/structures/ring_queue.h>

#include <stdint.h>

struct bsal_actor;

#define BSAL_SCHEDULING_QUEUE_PRIORITY_LOW 4
#define BSAL_SCHEDULING_QUEUE_PRIORITY_NORMAL 64
#define BSAL_SCHEDULING_QUEUE_PRIORITY_HIGH 128
#define BSAL_SCHEDULING_QUEUE_PRIORITY_MAX 1048576

#define BSAL_SCHEDULING_QUEUE_RATIO 64

/*
 * This is an actor scheduling queue.
 * Each worker has one of these.
 */
struct bsal_scheduling_queue {

    uint64_t max_priority_dequeue_operations;
    uint64_t high_priority_dequeue_operations;
    uint64_t normal_priority_dequeue_operations;
    uint64_t low_priority_dequeue_operations;

    struct bsal_ring_queue max_priority_queue;
    struct bsal_ring_queue high_priority_queue;
    struct bsal_ring_queue normal_priority_queue;
    struct bsal_ring_queue low_priority_queue;
};

void bsal_scheduling_queue_init(struct bsal_scheduling_queue *queue);
void bsal_scheduling_queue_destroy(struct bsal_scheduling_queue *queue);

int bsal_scheduling_queue_enqueue(struct bsal_scheduling_queue *queue, struct bsal_actor *actor);
int bsal_scheduling_queue_dequeue(struct bsal_scheduling_queue *queue, struct bsal_actor **actor);

int bsal_scheduling_queue_size(struct bsal_scheduling_queue *queue);

#endif
