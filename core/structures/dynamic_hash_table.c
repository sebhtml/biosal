
#include "dynamic_hash_table.h"

#include <core/system/tracer.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/*
#define BSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
#define BSAL_DYNAMIC_HASH_TABLE_DEBUG_RESIZING
*/


/* options */
/*#define BSAL_DYNAMIC_HASH_TABLE_THRESHOLD 0.90*/
/*#define BSAL_DYNAMIC_HASH_TABLE_THRESHOLD 0.75*/

#define BSAL_DYNAMIC_HASH_TABLE_THRESHOLD 0.70

void bsal_dynamic_hash_table_init(struct bsal_dynamic_hash_table *self, uint64_t buckets,
                int key_size, int value_size)
{
#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("DEBUG bsal_dynamic_hash_table_init buckets %d key_size %d value_size %d\n",
                    (int)buckets, key_size, value_size);
#endif

    bsal_dynamic_hash_table_reset(self);

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("value size %d\n", value_size);
#endif

    bsal_hash_table_init(self->current, buckets, key_size, value_size);
    buckets = bsal_hash_table_buckets(self->current);
    self->resize_next_size = 2 * buckets;
    self->resize_in_progress = 0;

    bsal_dynamic_hash_table_set_threshold(self, BSAL_DYNAMIC_HASH_TABLE_THRESHOLD);
}

void bsal_dynamic_hash_table_destroy(struct bsal_dynamic_hash_table *self)
{
    bsal_hash_table_destroy(self->current);

    if (self->resize_in_progress) {
        bsal_hash_table_destroy(self->next);
    }

    self->resize_in_progress = 0;
    self->current = NULL;
    self->next = NULL;
}

void bsal_dynamic_hash_table_reset(struct bsal_dynamic_hash_table *self)
{
    self->current = &self->table1;
    self->next = &self->table2;
    self->resize_in_progress = 0;
}

void *bsal_dynamic_hash_table_add(struct bsal_dynamic_hash_table *self, void *key)
{
    float ratio;
    float threshold;
    void *bucket;
    void *new_bucket;
    int value_size;

    /* if no resize is in progress, the load is verified
     */
    if (!self->resize_in_progress) {

        threshold = self->resize_load_threshold;
        ratio = (0.0 + bsal_hash_table_size(self->current)) / bsal_hash_table_buckets(self->current);

        if (ratio < threshold) {
            /* there is still place in the table.
             */
            return bsal_hash_table_add(self->current, key);

        } else {

            /* the resizing must begin
             */
#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
            printf("DEBUG bsal_dynamic_hash_table_add start resizing\n");
#endif

            bsal_dynamic_hash_table_start_resizing(self);

            /* perform a fancy recursive call
             */
            return bsal_dynamic_hash_table_add(self, key);
        }
    }

    /* if the resizing finished, just add the item to current
     */
    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_hash_table_add(self->current, key);
    }

    /*
     * the next table has the key
     * don't add anything.
     */
    bucket = bsal_hash_table_get(self->next, key);
    if (bucket != NULL) {
        return bucket;
    }

    /*
     * Otherwise, check if it is in the old one
     */

    bucket = bsal_hash_table_get(self->current, key);

    /*
     * If it is not in the current, simply add it to
     * the next
     */
    if (bucket == NULL) {
        return bsal_hash_table_add(self->next, key);
    }

    /* Otherwise, add it to the next, copy the value,
     * and return the bucket from next
     */

    new_bucket = bsal_hash_table_add(self->next, key);
    value_size = bsal_hash_table_value_size(self->current);

    if (value_size > 0) {
        memcpy(new_bucket, bucket, value_size);
    }

    self->resize_current_size--;

    return new_bucket;
}

void *bsal_dynamic_hash_table_get(struct bsal_dynamic_hash_table *self, void *key)
{
    void *bucket;

    if (!self->resize_in_progress) {
        return bsal_hash_table_get(self->current, key);
    }

    /* the resizing is finished, everything is in current
     */
    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_hash_table_get(self->current, key);
    }

    /*
     * First, look in the next table
     */
    bucket = bsal_hash_table_get(self->next, key);

    if (bucket != NULL) {
        return bucket;
    }

    return bsal_hash_table_get(self->current, key);
}

void bsal_dynamic_hash_table_delete(struct bsal_dynamic_hash_table *self, void *key)
{
    void *bucket;

    if (!self->resize_in_progress) {
        bsal_hash_table_delete(self->current, key);
        return;
    }

    /* if the resizing is completed, everything is in current
     */
    if (bsal_dynamic_hash_table_resize(self)) {

        bsal_hash_table_delete(self->current, key);
        return;
    }

    /* First look for the key in
     * the next table
     */
    bucket = bsal_hash_table_get(self->next, key);
    if (bucket != NULL) {
        bsal_hash_table_delete(self->next, key);
        return;
    }

    bsal_hash_table_delete(self->current, key);
}

uint64_t bsal_dynamic_hash_table_size(struct bsal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_size(self->current);
    }

    /* Resize, if that completes, the size is simply
     * the size of the current table
     */
    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_hash_table_size(self->current);
    }

    return self->resize_current_size + bsal_hash_table_size(self->next);
}

uint64_t bsal_dynamic_hash_table_buckets(struct bsal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_buckets(self->current);
    }

    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_hash_table_buckets(self->current);
    }

    return bsal_hash_table_buckets(self->current) + bsal_hash_table_buckets(self->next);
}

void bsal_dynamic_hash_table_start_resizing(struct bsal_dynamic_hash_table *self)
{
    /*uint64_t old_size;*/
    uint64_t new_size;

    /* already resizing
     */
    if (self->resize_in_progress) {
        return;
    }

    new_size = self->resize_next_size;
    /*printf("NEW SIZE %" PRIu64 "\n", new_size);*/
    self->resize_next_size = 2 * new_size;

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG_RESIZING
    old_size = bsal_hash_table_buckets(self->current);
    printf("DEBUG bsal_dynamic_hash_table_start_resizing start resizing %d ... %d\n",
                    (int)old_size, (int)new_size);
#endif

    self->resize_in_progress = 1;
    self->resize_current_size = bsal_hash_table_size(self->current);

    bsal_hash_table_init(self->next, new_size,
                    bsal_hash_table_key_size(self->current),
                    bsal_hash_table_value_size(self->current));

    /*
     * Disable the deletion support if necessary
     */
    if (!bsal_hash_table_deletion_support_is_enabled(self->current)) {
        bsal_hash_table_disable_deletion_support(self->next);
    }

    /*
     * Transfer the memory pool to the new one too.
     */
    bsal_hash_table_set_memory_pool(self->next,
                    bsal_hash_table_memory_pool(self->current));

    bsal_hash_table_iterator_init(&self->iterator, self->current);

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("DEBUG current %p %d/%d, next %p %d/%d\n",
                    (void *)self->current,
                        (int)bsal_hash_table_size(self->current),
                        (int)bsal_hash_table_buckets(self->current),
                        (void *)self->next,
                        (int)bsal_hash_table_size(self->next),
                        (int)bsal_hash_table_buckets(self->next));
#endif
}

int bsal_dynamic_hash_table_resize(struct bsal_dynamic_hash_table *self)
{
    int count;
    struct bsal_hash_table *table;
    void *key;
    void *value;
    void *new_value;
    int value_size;

    if (!self->resize_in_progress) {
        return 0;
    }

    value_size = bsal_hash_table_value_size(self->current);

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("value_size = %d\n", value_size);
#endif

    /* transfer N elements in the next table
     */
    count = 2;
    while (count-- && bsal_hash_table_iterator_has_next(&self->iterator)) {

        if (value_size == 0) {
            bsal_hash_table_iterator_next(&self->iterator, &key, NULL);

            if (bsal_hash_table_get(self->next, key) == NULL) {
                /* The key may already be there, but since the value size
                 * is 0, it does not matter
                 */
                bsal_hash_table_add(self->next, key);

                self->resize_current_size--;
            }

        } else {
            bsal_hash_table_iterator_next(&self->iterator, &key, &value);

            /* Only copy the value if it is not already there.
             */
            if (bsal_hash_table_get(self->next, key) == NULL) {
                new_value = bsal_hash_table_add(self->next, key);

                if (new_value == NULL) {
                    printf("Error, it is full\n");
                    printf("current %" PRIu64 "/%" PRIu64 "\n", bsal_hash_table_size(self->current), bsal_hash_table_buckets(self->current));
                    printf("next %" PRIu64 "/%" PRIu64 "\n", bsal_hash_table_size(self->next), bsal_hash_table_buckets(self->next));
                }

                memcpy(new_value, value, value_size);

                self->resize_current_size--;
            }
        }

        /* Remove the old copy from current.
         * This is not required in this algorithm,
         * regardless if deletion support is enabled.
         */
#if 0
        bsal_hash_table_delete(self->current, key);
#endif
    }

    if (!bsal_hash_table_iterator_has_next(&self->iterator)) {

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
        printf("DEBUG %p bsal_dynamic_hash_table_resize completed.\n",
                        (void *)self);

        printf("DEBUG current %p %d/%d, next %p %d/%d\n",
                    (void *)self->current,
                        (int)bsal_hash_table_size(self->current),
                        (int)bsal_hash_table_buckets(self->current),
                        (void *)self->next,
                        (int)bsal_hash_table_size(self->next),
                        (int)bsal_hash_table_buckets(self->next));
#endif

        bsal_hash_table_iterator_destroy(&self->iterator);

        /* the transfer is finished, swap current and main
         */
        table = self->current;
        self->current = self->next;
        self->next = table;

        bsal_hash_table_destroy(self->next);

        self->resize_in_progress = 0;

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG_RESIZING
        printf("DEBUG bsal_dynamic_hash_table_resize resizing is done current %p next %p\n",
                        (void *)self->current, (void *)self->next);
#endif
        return 1;
    }

    return 0;
}

int bsal_dynamic_hash_table_state(struct bsal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_state(self->current, bucket);
    }

    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_hash_table_state(self->current, bucket);
    }

    if (bucket < bsal_hash_table_buckets(self->current)) {
        return bsal_hash_table_state(self->current, bucket);
    }

    return bsal_hash_table_state(self->next, bucket - bsal_hash_table_buckets(self->current));
}

void *bsal_dynamic_hash_table_key(struct bsal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_key(self->current, bucket);
    }

    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_hash_table_key(self->current, bucket);
    }

    if (bucket < bsal_hash_table_buckets(self->current)) {
        return bsal_hash_table_key(self->current, bucket);
    }

    return bsal_hash_table_key(self->next, bucket - bsal_hash_table_buckets(self->current));
}

void *bsal_dynamic_hash_table_value(struct bsal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_value(self->current, bucket);
    }

    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_hash_table_value(self->current, bucket);
    }

    if (bucket < bsal_hash_table_buckets(self->current)) {
        return bsal_hash_table_value(self->current, bucket);
    }

    return bsal_hash_table_value(self->next, bucket - bsal_hash_table_buckets(self->current));
}

int bsal_dynamic_hash_table_pack_size(struct bsal_dynamic_hash_table *self)
{
    if (self->resize_in_progress) {
        bsal_dynamic_hash_table_finish_resizing(self);
    }

    return bsal_hash_table_pack_size(self->current);
}

int bsal_dynamic_hash_table_pack(struct bsal_dynamic_hash_table *self, void *buffer)
{
    if (self->resize_in_progress) {
        bsal_dynamic_hash_table_finish_resizing(self);
    }

    return bsal_hash_table_pack(self->current, buffer);
}

int bsal_dynamic_hash_table_unpack(struct bsal_dynamic_hash_table *self, void *buffer)
{
    bsal_dynamic_hash_table_reset(self);

    return bsal_hash_table_unpack(self->current, buffer);
}

void bsal_dynamic_hash_table_finish_resizing(struct bsal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return;
    }

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG_RESIZING
    if (bsal_hash_table_size(self->current) == 0) {
        bsal_tracer_print_stack_backtrace();
    }
    printf("DEBUG finish resizing current %" PRIu64 "/%" PRIu64 " next %" PRIu64 "/%" PRIu64 "\n",
                    bsal_hash_table_size(self->current), bsal_hash_table_buckets(self->current),
        bsal_hash_table_size(self->next), bsal_hash_table_buckets(self->next));
#endif

    while (self->resize_in_progress) {
        bsal_dynamic_hash_table_resize(self);
    }
}

int bsal_dynamic_hash_table_get_key_size(struct bsal_dynamic_hash_table *self)
{
    return bsal_hash_table_key_size(self->current);
}

int bsal_dynamic_hash_table_get_value_size(struct bsal_dynamic_hash_table *self)
{
    return bsal_hash_table_value_size(self->current);
}

void bsal_dynamic_hash_table_set_memory_pool(struct bsal_dynamic_hash_table *table,
                struct bsal_memory_pool *memory)
{
    if (table->current != NULL) {
        bsal_hash_table_set_memory_pool(table->current, memory);
    }

    if (table->next != NULL) {
        bsal_hash_table_set_memory_pool(table->next, memory);
    }
}

void bsal_dynamic_hash_table_disable_deletion_support(struct bsal_dynamic_hash_table *table)
{
    if (table->current != NULL) {
        bsal_hash_table_disable_deletion_support(table->current);
    }

    if (table->next != NULL) {
        bsal_hash_table_disable_deletion_support(table->next);
    }

}

void bsal_dynamic_hash_table_enable_deletion_support(struct bsal_dynamic_hash_table *table)
{
    if (table->current != NULL) {
        bsal_hash_table_enable_deletion_support(table->current);
    }

    if (table->next != NULL) {
        bsal_hash_table_enable_deletion_support(table->next);
    }
}

void bsal_dynamic_hash_table_set_current_size_estimate(struct bsal_dynamic_hash_table *table,
                double value)
{
    uint64_t current_size;
    double threshold;
    uint64_t size_estimate;
    uint64_t next_size;
    uint64_t required;
    int avoided_resizing_operations;
    uint64_t current_buckets;

    current_size = bsal_dynamic_hash_table_size(table);
    threshold = table->resize_load_threshold;
    current_buckets = bsal_hash_table_buckets(table->current);

    size_estimate = current_size * (1 / value);

    next_size = 2;
    required = size_estimate / (threshold - 0.01);

    while (next_size < required) {
        next_size *= 2;
    }

    printf("ESTIMATE estimate %f current_size %" PRIu64 "/%" PRIu64 " size_estimate %" PRIu64 " threshold %f required %" PRIu64 " next_size %" PRIu64 "\n",
                    value, current_size, current_buckets,
                    size_estimate, threshold, required, next_size);

    if (next_size > table->resize_next_size) {
        avoided_resizing_operations = 0;

        while (current_buckets < next_size) {
            current_buckets *= 2;
            ++avoided_resizing_operations;
        }

        avoided_resizing_operations -= 1;
        printf("OPTIMIZATION resize_next_size... old value %" PRIu64 " new value %" PRIu64 " (avoided resizing operations: %d)\n",
                    table->resize_next_size, next_size, avoided_resizing_operations);

        table->resize_next_size = next_size;
    }
}

void bsal_dynamic_hash_table_set_threshold(struct bsal_dynamic_hash_table *table, double threshold)
{
    table->resize_load_threshold = threshold;
}

int bsal_dynamic_hash_table_is_currently_resizing(struct bsal_dynamic_hash_table *table)
{
    return table->resize_in_progress;
}

void bsal_dynamic_hash_table_clear(struct bsal_dynamic_hash_table *self)
{
    bsal_dynamic_hash_table_finish_resizing(self);

    bsal_hash_table_clear(self->current);
}
