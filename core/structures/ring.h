
#ifndef BIOSAL_RING_H
#define BIOSAL_RING_H

/*
 * \see http://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular
 */
struct biosal_ring {
    void *cells;
    int number_of_cells;
    int head;
    int tail;
    int cell_size;
};

void biosal_ring_init(struct biosal_ring *self, int capacity, int cell_size);
void biosal_ring_destroy(struct biosal_ring *self);

int biosal_ring_push(struct biosal_ring *self, void *element);
int biosal_ring_pop(struct biosal_ring *self, void *element);

int biosal_ring_is_full(struct biosal_ring *self);
int biosal_ring_is_empty(struct biosal_ring *self);

int biosal_ring_size(struct biosal_ring *self);
int biosal_ring_capacity(struct biosal_ring *self);

int biosal_ring_increment(struct biosal_ring *self, int index);
void *biosal_ring_get_cell(struct biosal_ring *self, int index);

#endif
