
#ifndef BSAL_SCHEDULING_QUEUE
#define BSAL_SCHEDULING_QUEUE

#include <core/structures/ring_queue.h>

#include <stdint.h>

struct thorium_actor;

/*
 * \see http://dictionary.cambridge.org/dictionary/british/max_1
 */
#define BSAL_PRIORITY_LOW 4
#define BSAL_PRIORITY_NORMAL 64
#define BSAL_PRIORITY_HIGH 128
#define BSAL_PRIORITY_MAX 1048576

#define BSAL_SCHEDULING_QUEUE_RATIO 64

/*
 * This is an actor scheduling queue.
 * Each worker has one of these.
 */
struct thorium_scheduling_queue {

    uint64_t max_priority_dequeue_operations;
    uint64_t high_priority_dequeue_operations;
    uint64_t normal_priority_dequeue_operations;
    uint64_t low_priority_dequeue_operations;

    struct bsal_ring_queue max_priority_queue;
    struct bsal_ring_queue high_priority_queue;
    struct bsal_ring_queue normal_priority_queue;
    struct bsal_ring_queue low_priority_queue;
};

void thorium_scheduling_queue_init(struct thorium_scheduling_queue *self);
void thorium_scheduling_queue_destroy(struct thorium_scheduling_queue *self);

int thorium_scheduling_queue_enqueue(struct thorium_scheduling_queue *self, struct thorium_actor *actor);
int thorium_scheduling_queue_dequeue(struct thorium_scheduling_queue *self, struct thorium_actor **actor);

int thorium_scheduling_queue_size(struct thorium_scheduling_queue *self);

int thorium_scheduling_queue_get_size_with_priority(struct thorium_scheduling_queue *self, int priority);

struct bsal_ring_queue *thorium_scheduling_queue_select_queue(struct thorium_scheduling_queue *self, int priority);
uint64_t *thorium_scheduling_queue_select_counter(struct thorium_scheduling_queue *self, int priority);

int thorium_scheduling_queue_dequeue_with_priority(struct thorium_scheduling_queue *self, int priority,
                struct thorium_actor **actor);

void thorium_scheduling_queue_reset_counter(struct thorium_scheduling_queue *self, int priority);
uint64_t thorium_scheduling_queue_get_counter(struct thorium_scheduling_queue *self, int priority);

void thorium_scheduling_queue_print(struct thorium_scheduling_queue *self, int node, int worker);
void thorium_scheduling_queue_print_with_priority(struct thorium_scheduling_queue *self, int priority, const char *name,
                int node, int worker);

#endif
