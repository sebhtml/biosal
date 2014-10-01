
#ifndef CORE_RING_H
#define CORE_RING_H

/*
 * \see http://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular
 */
struct core_ring {
    void *cells;
    int number_of_cells;
    int head;
    int tail;
    int cell_size;
};

void core_ring_init(struct core_ring *self, int capacity, int cell_size);
void core_ring_destroy(struct core_ring *self);

int core_ring_push(struct core_ring *self, void *element);
int core_ring_pop(struct core_ring *self, void *element);

int core_ring_is_full(struct core_ring *self);
int core_ring_is_empty(struct core_ring *self);

int core_ring_size(struct core_ring *self);
int core_ring_capacity(struct core_ring *self);

int core_ring_increment(struct core_ring *self, int index);
void *core_ring_get_cell(struct core_ring *self, int index);

#endif
