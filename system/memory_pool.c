
#include "memory_pool.h"

#include <structures/queue.h>
#include <structures/map_iterator.h>
#include <structures/vector_iterator.h>

void bsal_memory_pool_init(struct bsal_memory_pool *self, int block_size)
{
    bsal_map_init(&self->recycle_bin, sizeof(int), sizeof(struct bsal_queue));
    bsal_map_init(&self->allocated_blocks, sizeof(void *), sizeof(int));
    self->current_block = NULL;

    bsal_vector_init(&self->dried_blocks, sizeof(struct bsal_memory_block *));

    self->block_size = block_size;
}

void bsal_memory_pool_destroy(struct bsal_memory_pool *self)
{
    struct bsal_queue *queue;
    struct bsal_map_iterator iterator;
    struct bsal_vector_iterator vector_iterator;
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
    bsal_map_destroy(&self->allocated_blocks);

    /* destroy dried blocks
     */
    bsal_vector_iterator_init(&vector_iterator, &self->dried_blocks);

    while (bsal_vector_iterator_has_next(&vector_iterator)) {
        bsal_vector_iterator_next(&vector_iterator, (void **)&block);

        bsal_memory_block_destroy(block);
    }
    bsal_vector_iterator_destroy(&vector_iterator);
    bsal_vector_destroy(&self->dried_blocks);

    if (self->current_block != NULL) {
        bsal_memory_block_destroy(self->current_block);
        self->current_block = NULL;
    }
}

void *bsal_memory_pool_allocate(struct bsal_memory_pool *self, int size)
{
    struct bsal_queue *queue;
    void *pointer;

    queue = bsal_map_get(&self->recycle_bin, &size);

    /* recycling is good for the environment
     */
    if (queue != NULL && bsal_queue_dequeue(queue, &pointer)) {
        bsal_map_add_value(&self->allocated_blocks, &pointer, &size);

#ifdef BSAL_MEMORY_POOL_DISCARD_EMPTY_QUEUES
        if (bsal_queue_empty(queue)) {
            bsal_queue_destroy(queue);
            bsal_map_delete(&self->recycle_bin, &size);
        }
#endif

        return pointer;
    }

    if (self->current_block == NULL) {
        self->current_block = bsal_allocate(sizeof(struct bsal_memory_block));

        bsal_memory_block_init(self->current_block, self->block_size);
    }

    pointer = bsal_memory_block_allocate(self->current_block, size);

    /* the current block is exausted...
     */
    if (pointer == NULL) {
        bsal_vector_push_back(&self->dried_blocks, &self->current_block);
        self->current_block = NULL;
        return bsal_memory_pool_allocate(self, size);
    }

    bsal_map_add_value(&self->allocated_blocks, &pointer, &size);

    return pointer;
}

void bsal_memory_pool_free(struct bsal_memory_pool *self, void *pointer)
{
    struct bsal_queue *queue;
    int size;

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


