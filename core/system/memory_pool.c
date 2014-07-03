
#include "memory_pool.h"

#include <core/structures/queue.h>
#include <core/structures/map_iterator.h>

void bsal_memory_pool_init(struct bsal_memory_pool *self, int block_size)
{
    bsal_map_init(&self->recycle_bin, sizeof(int), sizeof(struct bsal_queue));
    bsal_map_init(&self->allocated_blocks, sizeof(void *), sizeof(int));

    self->current_block = NULL;

    bsal_queue_init(&self->dried_blocks, sizeof(struct bsal_memory_block *));
    bsal_queue_init(&self->ready_blocks, sizeof(struct bsal_memory_block *));

    self->block_size = block_size;
    self->tracking_is_enabled = 1;

    self->disabled = 0;
}

void bsal_memory_pool_destroy(struct bsal_memory_pool *self)
{
    struct bsal_queue *queue;
    struct bsal_map_iterator iterator;
    struct bsal_memory_block *block;

    /* destroy recycled objects
     */
    bsal_map_iterator_init(&iterator, &self->recycle_bin);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, NULL, (void **)&queue);

        bsal_queue_destroy(queue);
    }
    bsal_map_iterator_destroy(&iterator);
    bsal_map_destroy(&self->recycle_bin);

    /* destroy allocated blocks */
    bsal_map_destroy(&self->allocated_blocks);

    /* destroy dried blocks
     */
    while (bsal_queue_dequeue(&self->dried_blocks, &block)) {
        bsal_memory_block_destroy(block);
    }
    bsal_queue_destroy(&self->dried_blocks);

    /* destroy ready blocks
     */
    while (bsal_queue_dequeue(&self->ready_blocks, &block)) {
        bsal_memory_block_destroy(block);
    }
    bsal_queue_destroy(&self->ready_blocks);

    /* destroy the current block
     */
    if (self->current_block != NULL) {
        bsal_memory_block_destroy(self->current_block);
        self->current_block = NULL;
    }
}

void *bsal_memory_pool_allocate(struct bsal_memory_pool *self, int size)
{
    struct bsal_queue *queue;
    void *pointer;

#ifdef BSAL_MEMORY_ALIGNMENT_ENABLED
    /* Align memory to avoid problems with performance and/or
     * Bus errors...
     */
    size = bsal_memory_align(size);
#endif

    if (self->disabled) {
        return bsal_memory_allocate(size);
    }

    queue = NULL;

    if (self->tracking_is_enabled) {
        queue = bsal_map_get(&self->recycle_bin, &size);
    }

    /* recycling is good for the environment
     */
    if (queue != NULL && bsal_queue_dequeue(queue, &pointer)) {

        if (self->tracking_is_enabled) {
            bsal_map_add_value(&self->allocated_blocks, &pointer, &size);
        }

#ifdef BSAL_MEMORY_POOL_DISCARD_EMPTY_QUEUES
        if (bsal_queue_empty(queue)) {
            bsal_queue_destroy(queue);
            bsal_map_delete(&self->recycle_bin, &size);
        }
#endif

        return pointer;
    }

    if (self->current_block == NULL) {

        /* Try to pick a block in the ready block list.
         * Otherwise, create one on-demand today.
         */
        if (!bsal_queue_dequeue(&self->ready_blocks, &self->current_block)) {
            self->current_block = bsal_memory_allocate(sizeof(struct bsal_memory_block));
            bsal_memory_block_init(self->current_block, self->block_size);
        }
    }

    pointer = bsal_memory_block_allocate(self->current_block, size);

    /* the current block is exausted...
     */
    if (pointer == NULL) {
        bsal_queue_enqueue(&self->dried_blocks, &self->current_block);
        self->current_block = NULL;
        return bsal_memory_pool_allocate(self, size);
    }

    if (self->tracking_is_enabled) {
        bsal_map_add_value(&self->allocated_blocks, &pointer, &size);
    }

    return pointer;
}

void bsal_memory_pool_free(struct bsal_memory_pool *self, void *pointer)
{
    struct bsal_queue *queue;
    int size;

    if (self->disabled) {
        bsal_memory_free(pointer);
        return;
    }

    if (!self->tracking_is_enabled) {
        return;
    }

    if (!bsal_map_get_value(&self->allocated_blocks, &pointer, &size)) {
        return;
    }

    queue = bsal_map_get(&self->recycle_bin, &size);

    if (queue == NULL) {
        queue = bsal_map_add(&self->recycle_bin, &size);
        bsal_queue_init(queue, sizeof(void *));
    }

    bsal_queue_enqueue(queue, &pointer);

    bsal_map_delete(&self->allocated_blocks, &pointer);
}

void bsal_memory_pool_disable_tracking(struct bsal_memory_pool *self)
{
    self->tracking_is_enabled = 0;
}

void bsal_memory_pool_enable_tracking(struct bsal_memory_pool *self)
{
    self->tracking_is_enabled = 1;
}

void bsal_memory_pool_free_all(struct bsal_memory_pool *self)
{
    struct bsal_memory_block *block;
    int i;
    int size;

    /* reset the current block
     */
    if (self->current_block != NULL) {
        bsal_memory_block_free_all(self->current_block);
    }

    /* reset all ready blocks
     */

    size = bsal_queue_size(&self->ready_blocks);
    i = 0;
    while (i < size
                   && bsal_queue_dequeue(&self->ready_blocks, &block)) {
        bsal_memory_block_free_all(block);
        bsal_queue_enqueue(&self->ready_blocks, &block);

        i++;
    }

    /* reset all dried blocks */
    while (bsal_queue_dequeue(&self->dried_blocks, &block)) {
        bsal_memory_block_free_all(block);
        bsal_queue_enqueue(&self->ready_blocks, &block);
    }
}

void bsal_memory_pool_disable(struct bsal_memory_pool *self)
{
    self->disabled = 1;
}
