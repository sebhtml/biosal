
#ifndef BSAL_RING_H
#define BSAL_RING_H

/*
 * \see http://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular
 */
struct bsal_ring {
    void *cells;
    int number_of_cells;
    int head;
    int tail;
    int cell_size;
};

void bsal_ring_init(struct bsal_ring *self, int capacity, int cell_size);
void bsal_ring_destroy(struct bsal_ring *self);

int bsal_ring_push(struct bsal_ring *self, void *element);
int bsal_ring_pop(struct bsal_ring *self, void *element);

int bsal_ring_is_full(struct bsal_ring *self);
int bsal_ring_is_empty(struct bsal_ring *self);

int bsal_ring_size(struct bsal_ring *self);
int bsal_ring_capacity(struct bsal_ring *self);

int bsal_ring_increment(struct bsal_ring *self, int index);
void *bsal_ring_get_cell(struct bsal_ring *self, int index);

#endif
