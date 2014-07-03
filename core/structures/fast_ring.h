
#ifndef BSAL_FAST_RING_H
#define BSAL_FAST_RING_H

struct bsal_fast_ring;

#include <stdint.h>

/*
*/
#define BSAL_FAST_RING_VOLATILE

/*
#define BSAL_FAST_RING_ATOMIC
*/

/*
 * \see http://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular
 */
struct bsal_fast_ring {
    /*
     * \see http://stackoverflow.com/questions/19744508/volatile-vs-atomic
     * \see http://stackoverflow.com/questions/6397662/if-volatile-is-useless-for-threading-why-do-atomic-operations-require-pointers
     * \see https://software.intel.com/en-us/blogs/2007/11/30/volatile-almost-useless-for-multi-threaded-programming/
     * \see http://blogs.msdn.com/b/ericlippert/archive/2011/06/16/atomicity-volatility-and-immutability-are-different-part-three.aspx
     * \see http://stackoverflow.com/questions/4557979/when-to-use-volatile-with-multi-threading/4558031#4558031
     */

    /*
     * None of these variable is volatile
     * because volatile gives bad performance overall
     */
    uint64_t head;
    uint64_t tail_cache;
    uint64_t consumer_padding_0;
    uint64_t consumer_padding_1;
    uint64_t consumer_padding_2;
    uint64_t consumer_padding_3;
    uint64_t consumer_padding_4;
    uint64_t consumer_padding_5;

    uint64_t tail;
    uint64_t head_cache;
    uint64_t producer_padding_0;
    uint64_t producer_padding_1;
    uint64_t producer_padding_2;
    uint64_t producer_padding_3;
    uint64_t producer_padding_4;
    uint64_t producer_padding_5;

    void *cells;
    uint64_t number_of_cells;
    uint64_t mask;
    int cell_size;
};

void bsal_fast_ring_init(struct bsal_fast_ring *self, int capacity, int cell_size);
void bsal_fast_ring_destroy(struct bsal_fast_ring *self);

int bsal_fast_ring_push_from_producer(struct bsal_fast_ring *self, void *element);
int bsal_fast_ring_pop_from_consumer(struct bsal_fast_ring *self, void *element);

void *bsal_fast_ring_get_cell(struct bsal_fast_ring *self, uint64_t index);

uint64_t bsal_fast_ring_increment(struct bsal_fast_ring *self, uint64_t index);
int bsal_fast_ring_capacity(struct bsal_fast_ring *self);

int bsal_fast_ring_get_next_power_of_two(int value);
int bsal_fast_ring_is_full_from_producer(struct bsal_fast_ring *self);
int bsal_fast_ring_is_empty_from_consumer(struct bsal_fast_ring *self);

int bsal_fast_ring_size_from_consumer(struct bsal_fast_ring *self);
int bsal_fast_ring_size_from_producer(struct bsal_fast_ring *self);

void bsal_fast_ring_update_head_cache(struct bsal_fast_ring *self);
void bsal_fast_ring_update_tail_cache(struct bsal_fast_ring *self);

uint64_t bsal_fast_ring_mock(struct bsal_fast_ring *self);

#endif
