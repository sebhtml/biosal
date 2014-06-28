
#ifndef BSAL_RING_H
#define BSAL_RING_H

struct bsal_ring;

/*
#define BSAL_RING_VOLATILE
*/

/*
#define BSAL_RING_ATOMIC
*/

/*
 * \see http://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular
 */
struct bsal_ring {
    void *cells;
    int number_of_cells;
    int cell_size;

    /*
     * \see http://stackoverflow.com/questions/19744508/volatile-vs-atomic
     * \see http://stackoverflow.com/questions/6397662/if-volatile-is-useless-for-threading-why-do-atomic-operations-require-pointers
     * \see https://software.intel.com/en-us/blogs/2007/11/30/volatile-almost-useless-for-multi-threaded-programming/
     * \see http://blogs.msdn.com/b/ericlippert/archive/2011/06/16/atomicity-volatility-and-immutability-are-different-part-three.aspx
     * \see http://stackoverflow.com/questions/4557979/when-to-use-volatile-with-multi-threading/4558031#4558031
     */

#ifdef BSAL_RING_VOLATILE
    volatile int head;
    volatile int tail;
#else
    volatile int head;
    volatile int tail;
#endif
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

int bsal_ring_get_head(struct bsal_ring *self);
int bsal_ring_get_tail(struct bsal_ring *self);
void bsal_ring_increment_head(struct bsal_ring *self);
void bsal_ring_increment_tail(struct bsal_ring *self);

#endif
