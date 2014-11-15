
#ifndef CORE_SIMPLE_QUEUE_H
#define CORE_SIMPLE_QUEUE_H

struct core_memory_pool;

struct core_simple_queue_item {
    struct core_simple_queue_item *next_;
    void *data_;
};

/*
 * A simple queue.
 *
 * This implementation uses a linked list and enqueue and dequeue
 * are O(1).
 */
struct core_simple_queue {
    struct core_simple_queue_item *tail_;
    struct core_simple_queue_item *head_;
    struct core_simple_queue_item *garbage_;
    struct core_simple_queue_item *allocations_;
    struct core_memory_pool *pool_;

    int bytes_per_unit_;
    int size_;
    int garbage_mode_;
};

void core_simple_queue_init(struct core_simple_queue *self, int bytes_per_unit);
void core_simple_queue_destroy(struct core_simple_queue *self);

int core_simple_queue_enqueue(struct core_simple_queue *self, void *data);
int core_simple_queue_dequeue(struct core_simple_queue *self, void *data);

int core_simple_queue_empty(struct core_simple_queue *self);
int core_simple_queue_full(struct core_simple_queue *self);
int core_simple_queue_size(struct core_simple_queue *self);

int core_simple_queue_capacity(struct core_simple_queue *self);
void core_simple_queue_set_memory_pool(struct core_simple_queue *self,
                struct core_memory_pool *pool);
void core_simple_queue_set_garbage_mode(struct core_simple_queue *self);

#endif
