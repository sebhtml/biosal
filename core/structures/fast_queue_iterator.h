
#ifndef CORE_FAST_QUEUE_ITERATOR_H
#define CORE_FAST_QUEUE_ITERATOR_H

struct core_fast_queue;

struct core_fast_queue_iterator {
    int i;
    int size;
    struct core_fast_queue *container;
};

void core_fast_queue_iterator_init(struct core_fast_queue_iterator *self, struct core_fast_queue *container);
void core_fast_queue_iterator_destroy(struct core_fast_queue_iterator *self);

int core_fast_queue_iterator_has_next(struct core_fast_queue_iterator *self);
int core_fast_queue_iterator_next_value(struct core_fast_queue_iterator *self, void *value);

#endif
