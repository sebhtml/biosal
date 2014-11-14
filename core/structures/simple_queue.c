
#include "simple_queue.h"

#include <core/system/memory_pool.h>
#include <core/system/memory.h>
#include <core/system/debugger.h>

#include <stdlib.h>

/*
*/
#define USE_BLOCK_ALLOCATION

/*
#define DEBUG_QUEUE
*/

struct core_simple_queue_item *core_simple_queue_allocate_item(struct core_simple_queue *self);
void core_simple_queue_free_item(struct core_simple_queue *self,
                struct core_simple_queue_item *item);

void core_simple_queue_init(struct core_simple_queue *self, int bytes_per_unit)
{
    self->head_ = NULL;
    self->tail_ = NULL;
    self->garbage_ = NULL;
    self->allocations_ = NULL;
    self->pool_ = NULL;

    self->bytes_per_unit_ = bytes_per_unit;
    self->size_ = 0;
}

void core_simple_queue_destroy(struct core_simple_queue *self)
{
    struct core_simple_queue_item *item;
    struct core_simple_queue_item *next;

    item = self->allocations_;

    while (item != NULL) {
        next = item->next_;
        core_memory_pool_free(self->pool_, item);
        item = next;
    }

    self->head_ = NULL;
    self->tail_ = NULL;
    self->garbage_ = NULL;
    self->allocations_ = NULL;
    self->pool_ = NULL;
    self->size_ = 0;
    self->bytes_per_unit_ = 0;
}

int core_simple_queue_enqueue(struct core_simple_queue *self, void *data)
{
    struct core_simple_queue_item *new_item;

    /*
     * Allocate an item and copy the data.
     */
    new_item = core_simple_queue_allocate_item(self);

    CORE_DEBUGGER_ASSERT(new_item != NULL);
    CORE_DEBUGGER_ASSERT(new_item->data_ != NULL);
    CORE_DEBUGGER_ASSERT(new_item->next_ == NULL);

    core_memory_copy(new_item->data_, data, self->bytes_per_unit_);

    ++self->size_;

    /*
     * The queue is empty.
     */
    if (self->head_ == NULL && self->tail_ == NULL) {
        self->tail_ = new_item;
        self->head_ = self->tail_;
        return 1;
    }

    /*
     * Otherwise, there is at least one element in the queue.
     */

    self->tail_->next_ = new_item;
    self->tail_ = new_item;

    return 1;
}

int core_simple_queue_dequeue(struct core_simple_queue *self, void *data)
{
    struct core_simple_queue_item *item;

    if (!self->size_)
        return 0;

    --self->size_;

    item = self->head_;

    core_memory_copy(data, self->head_->data_, self->bytes_per_unit_);

    self->head_ = self->head_->next_;

    if (!self->size_)
        self->tail_ = NULL;

    core_simple_queue_free_item(self, item);

    return 1;
}

int core_simple_queue_empty(struct core_simple_queue *self)
{
    return !self->size_;
}

int core_simple_queue_full(struct core_simple_queue *self)
{
    return 0;
}

int core_simple_queue_size(struct core_simple_queue *self)
{
    return self->size_;
}

int core_simple_queue_capacity(struct core_simple_queue *self)
{
    return -1;
}

void core_simple_queue_set_memory_pool(struct core_simple_queue *self,
                struct core_memory_pool *pool)
{
    self->pool_ = pool;
}

struct core_simple_queue_item *core_simple_queue_allocate_item(struct core_simple_queue *self)
{
#ifdef USE_BLOCK_ALLOCATION
    struct core_simple_queue_item *item;
    int unit_size;
    int count;
    int block_size;
    int i;
    struct core_simple_queue_item *allocation;

    /*
     * Use the linked list of garbage-collected items.
     */
    if (self->garbage_ != NULL) {
        item = self->garbage_;

        /* Move the garbage pointer.
         */
        self->garbage_ = self->garbage_->next_;

        item->data_ = ((char *)item) + sizeof(struct core_simple_queue_item);
        item->next_ = NULL;

        return item;
    }

    unit_size = sizeof(struct core_simple_queue_item) + self->bytes_per_unit_;

    block_size = 16384;
    count = block_size / unit_size;

    block_size = count * unit_size;

    allocation = core_memory_pool_allocate(self->pool_, block_size);

    allocation->next_ = self->allocations_;
    self->allocations_ = allocation;

#ifdef DEBUG_QUEUE
    printf("generate %d items from %d bytes\n", count - 1, block_size);
#endif

    /*
     * Generate items in garbage list;
     */
    i = 0;

    /*
     * Skip the first one since it is used for tracking allocations.
     */
    ++i;
    while (i < count) {
        item = (void *)(((char *)allocation) + i * unit_size);
        item->next_ = self->garbage_;
        self->garbage_ = item;
        ++i;
    }

    /*
     * Recursive call.
     * There is at most one recursive call.
     */
    return core_simple_queue_allocate_item(self);

#else
    int unit_size;
    struct core_simple_queue_item *item;

    unit_size = sizeof(struct core_simple_queue_item) + self->bytes_per_unit_;

    item = core_memory_pool_allocate(self->pool_, unit_size);
    item->next_ = NULL;
    item->data_ = ((char *)item) + sizeof(struct core_simple_queue_item);

    return item;
#endif
}

void core_simple_queue_free_item(struct core_simple_queue *self,
                struct core_simple_queue_item *item)
{
#ifdef USE_BLOCK_ALLOCATION
    item->next_ = self->garbage_;
    self->garbage_ = item;
#else
    core_memory_pool_free(self->pool_, item);
#endif
}
