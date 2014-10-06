
#ifndef CORE_FAST_RING_H
#define CORE_FAST_RING_H

struct core_fast_ring;

#include <stdint.h>

/*
#define CORE_FAST_RING_VOLATILE
*/

/*
#define CORE_FAST_RING_ATOMIC
*/

/*
CORE_FAST_RING_USE_PADDING
*/
/*
 * \see http://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular
 *
 * \see http://mechanitis.blogspot.com/2011/08/dissecting-disruptor-why-its-so-fast.html
 *
 * \see http://stackoverflow.com/questions/17327095/how-does-disruptor-barriers-work
 */
struct core_fast_ring {
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

    /*
     * For consumer
     */
    uint64_t head;
#ifdef CORE_FAST_RING_USE_CACHE
    uint64_t tail_cache;
#endif

    /* PowerPC A2 has L1 cache line length of 64 bytes and L2 cache line length of
     * 128 bytes
     */

#ifdef CORE_FAST_RING_USE_PADDING
    uint64_t consumer_padding_0;
    uint64_t consumer_padding_1;
    uint64_t consumer_padding_2;
    uint64_t consumer_padding_3;
    uint64_t consumer_padding_4;
    uint64_t consumer_padding_5;
#endif

    /*
     * For producer
     */
    uint64_t tail;

#ifdef CORE_FAST_RING_USE_CACHE
    uint64_t head_cache;
#endif

#ifdef CORE_FAST_RING_USE_PADDING
    uint64_t producer_padding_0;
    uint64_t producer_padding_1;
    uint64_t producer_padding_2;
    uint64_t producer_padding_3;
    uint64_t producer_padding_4;
    uint64_t producer_padding_5;
#endif

    void *cells;
    uint64_t number_of_cells;
    uint64_t mask;
    int cell_size;
};

void core_fast_ring_init(struct core_fast_ring *self, int capacity, int cell_size);
void core_fast_ring_destroy(struct core_fast_ring *self);

int core_fast_ring_push_from_producer(struct core_fast_ring *self, void *element);
int core_fast_ring_pop_from_consumer(struct core_fast_ring *self, void *element);

int core_fast_ring_capacity(struct core_fast_ring *self);

int core_fast_ring_is_full_from_producer(struct core_fast_ring *self);
int core_fast_ring_is_empty_from_consumer(struct core_fast_ring *self);

int core_fast_ring_size_from_consumer(struct core_fast_ring *self);
int core_fast_ring_size_from_producer(struct core_fast_ring *self);

#ifdef CORE_FAST_RING_USE_CACHE
void core_fast_ring_update_head_cache(struct core_fast_ring *self);
void core_fast_ring_update_tail_cache(struct core_fast_ring *self);
#endif

int core_fast_ring_empty(struct core_fast_ring *self);

#endif
