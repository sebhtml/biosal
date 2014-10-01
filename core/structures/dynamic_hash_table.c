
#include "dynamic_hash_table.h"

#include <core/system/tracer.h>
#include <core/system/memory.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/*
#define BIOSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
#define BIOSAL_DYNAMIC_HASH_TABLE_DEBUG_RESIZING
*/


/* options */
/*#define BIOSAL_DYNAMIC_HASH_TABLE_THRESHOLD 0.90*/
/*#define BIOSAL_DYNAMIC_HASH_TABLE_THRESHOLD 0.75*/

#define BIOSAL_DYNAMIC_HASH_TABLE_THRESHOLD 0.70

void biosal_dynamic_hash_table_init(struct biosal_dynamic_hash_table *self, uint64_t buckets,
                int key_size, int value_size)
{
#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("DEBUG biosal_dynamic_hash_table_init buckets %d key_size %d value_size %d\n",
                    (int)buckets, key_size, value_size);
#endif

    biosal_dynamic_hash_table_reset(self);

#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("value size %d\n", value_size);
#endif

    biosal_hash_table_init(self->current, buckets, key_size, value_size);
    buckets = biosal_hash_table_buckets(self->current);
    self->resize_next_size = 2 * buckets;
    self->resize_in_progress = 0;

    biosal_dynamic_hash_table_set_threshold(self, BIOSAL_DYNAMIC_HASH_TABLE_THRESHOLD);
}

void biosal_dynamic_hash_table_destroy(struct biosal_dynamic_hash_table *self)
{
    biosal_hash_table_destroy(self->current);

    if (self->resize_in_progress) {
        biosal_hash_table_destroy(self->next);
    }

    self->resize_in_progress = 0;
    self->current = NULL;
    self->next = NULL;
}

void biosal_dynamic_hash_table_reset(struct biosal_dynamic_hash_table *self)
{
    self->current = &self->table1;
    self->next = &self->table2;
    self->resize_in_progress = 0;
}

void *biosal_dynamic_hash_table_add(struct biosal_dynamic_hash_table *self, void *key)
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
        ratio = (0.0 + biosal_hash_table_size(self->current)) / biosal_hash_table_buckets(self->current);

        if (ratio < threshold) {
            /* there is still place in the table.
             */
            return biosal_hash_table_add(self->current, key);

        } else {

            /* the resizing must begin
             */
#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
            printf("DEBUG biosal_dynamic_hash_table_add start resizing\n");
#endif

            biosal_dynamic_hash_table_start_resizing(self);

            /* perform a fancy recursive call
             */
            return biosal_dynamic_hash_table_add(self, key);
        }
    }

    /* if the resizing finished, just add the item to current
     */
    if (biosal_dynamic_hash_table_resize(self)) {
        return biosal_hash_table_add(self->current, key);
    }

    /*
     * the next table has the key
     * don't add anything.
     */
    bucket = biosal_hash_table_get(self->next, key);
    if (bucket != NULL) {
        return bucket;
    }

    /*
     * Otherwise, check if it is in the old one
     */

    bucket = biosal_hash_table_get(self->current, key);

    /*
     * If it is not in the current, simply add it to
     * the next
     */
    if (bucket == NULL) {
        return biosal_hash_table_add(self->next, key);
    }

    /* Otherwise, add it to the next, copy the value,
     * and return the bucket from next
     */

    new_bucket = biosal_hash_table_add(self->next, key);
    value_size = biosal_hash_table_value_size(self->current);

    if (value_size > 0) {
        biosal_memory_copy(new_bucket, bucket, value_size);
    }

    self->resize_current_size--;

    return new_bucket;
}

void *biosal_dynamic_hash_table_get(struct biosal_dynamic_hash_table *self, void *key)
{
    void *bucket;

    if (!self->resize_in_progress) {
        return biosal_hash_table_get(self->current, key);
    }

    /* the resizing is finished, everything is in current
     */
    if (biosal_dynamic_hash_table_resize(self)) {
        return biosal_hash_table_get(self->current, key);
    }

    /*
     * First, look in the next table
     */
    bucket = biosal_hash_table_get(self->next, key);

    if (bucket != NULL) {
        return bucket;
    }

    return biosal_hash_table_get(self->current, key);
}

void biosal_dynamic_hash_table_delete(struct biosal_dynamic_hash_table *self, void *key)
{
    void *bucket;

    if (!self->resize_in_progress) {
        biosal_hash_table_delete(self->current, key);
        return;
    }

    /* if the resizing is completed, everything is in current
     */
    if (biosal_dynamic_hash_table_resize(self)) {

        biosal_hash_table_delete(self->current, key);
        return;
    }

    /* First look for the key in
     * the next table
     */
    bucket = biosal_hash_table_get(self->next, key);
    if (bucket != NULL) {
        biosal_hash_table_delete(self->next, key);
        return;
    }

    biosal_hash_table_delete(self->current, key);
}

uint64_t biosal_dynamic_hash_table_size(struct biosal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return biosal_hash_table_size(self->current);
    }

    /* Resize, if that completes, the size is simply
     * the size of the current table
     */
    if (biosal_dynamic_hash_table_resize(self)) {
        return biosal_hash_table_size(self->current);
    }

    return self->resize_current_size + biosal_hash_table_size(self->next);
}

uint64_t biosal_dynamic_hash_table_buckets(struct biosal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return biosal_hash_table_buckets(self->current);
    }

    if (biosal_dynamic_hash_table_resize(self)) {
        return biosal_hash_table_buckets(self->current);
    }

    return biosal_hash_table_buckets(self->current) + biosal_hash_table_buckets(self->next);
}

void biosal_dynamic_hash_table_start_resizing(struct biosal_dynamic_hash_table *self)
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

#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG_RESIZING
    old_size = biosal_hash_table_buckets(self->current);
    printf("DEBUG biosal_dynamic_hash_table_start_resizing start resizing %d ... %d\n",
                    (int)old_size, (int)new_size);
#endif

    self->resize_in_progress = 1;
    self->resize_current_size = biosal_hash_table_size(self->current);

    biosal_hash_table_init(self->next, new_size,
                    biosal_hash_table_key_size(self->current),
                    biosal_hash_table_value_size(self->current));

    /*
     * Disable the deletion support if necessary
     */
    if (!biosal_hash_table_deletion_support_is_enabled(self->current)) {
        biosal_hash_table_disable_deletion_support(self->next);
    }

    /*
     * Transfer the memory pool to the new one too.
     */
    biosal_hash_table_set_memory_pool(self->next,
                    biosal_hash_table_memory_pool(self->current));

    biosal_hash_table_iterator_init(&self->iterator, self->current);

#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("DEBUG current %p %d/%d, next %p %d/%d\n",
                    (void *)self->current,
                        (int)biosal_hash_table_size(self->current),
                        (int)biosal_hash_table_buckets(self->current),
                        (void *)self->next,
                        (int)biosal_hash_table_size(self->next),
                        (int)biosal_hash_table_buckets(self->next));
#endif
}

int biosal_dynamic_hash_table_resize(struct biosal_dynamic_hash_table *self)
{
    int count;
    struct biosal_hash_table *table;
    void *key;
    void *value;
    void *new_value;
    int value_size;

    if (!self->resize_in_progress) {
        return 0;
    }

    value_size = biosal_hash_table_value_size(self->current);

#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("value_size = %d\n", value_size);
#endif

    /* transfer N elements in the next table
     */
    count = 2;
    while (count-- && biosal_hash_table_iterator_has_next(&self->iterator)) {

        if (value_size == 0) {
            biosal_hash_table_iterator_next(&self->iterator, &key, NULL);

            if (biosal_hash_table_get(self->next, key) == NULL) {
                /* The key may already be there, but since the value size
                 * is 0, it does not matter
                 */
                biosal_hash_table_add(self->next, key);

                self->resize_current_size--;
            }

        } else {
            biosal_hash_table_iterator_next(&self->iterator, &key, &value);

            /* Only copy the value if it is not already there.
             */
            if (biosal_hash_table_get(self->next, key) == NULL) {
                new_value = biosal_hash_table_add(self->next, key);

                if (new_value == NULL) {
                    printf("Error, it is full\n");
                    printf("current %" PRIu64 "/%" PRIu64 "\n", biosal_hash_table_size(self->current), biosal_hash_table_buckets(self->current));
                    printf("next %" PRIu64 "/%" PRIu64 "\n", biosal_hash_table_size(self->next), biosal_hash_table_buckets(self->next));
                }

                biosal_memory_copy(new_value, value, value_size);

                self->resize_current_size--;
            }
        }

        /* Remove the old copy from current.
         * This is not required in this algorithm,
         * regardless if deletion support is enabled.
         */
#if 0
        biosal_hash_table_delete(self->current, key);
#endif
    }

    if (!biosal_hash_table_iterator_has_next(&self->iterator)) {

#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
        printf("DEBUG %p biosal_dynamic_hash_table_resize completed.\n",
                        (void *)self);

        printf("DEBUG current %p %d/%d, next %p %d/%d\n",
                    (void *)self->current,
                        (int)biosal_hash_table_size(self->current),
                        (int)biosal_hash_table_buckets(self->current),
                        (void *)self->next,
                        (int)biosal_hash_table_size(self->next),
                        (int)biosal_hash_table_buckets(self->next));
#endif

        biosal_hash_table_iterator_destroy(&self->iterator);

        /* the transfer is finished, swap current and main
         */
        table = self->current;
        self->current = self->next;
        self->next = table;

        biosal_hash_table_destroy(self->next);

        self->resize_in_progress = 0;

#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG_RESIZING
        printf("DEBUG biosal_dynamic_hash_table_resize resizing is done current %p next %p\n",
                        (void *)self->current, (void *)self->next);
#endif
        return 1;
    }

    return 0;
}

int biosal_dynamic_hash_table_state(struct biosal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return biosal_hash_table_state(self->current, bucket);
    }

    if (biosal_dynamic_hash_table_resize(self)) {
        return biosal_hash_table_state(self->current, bucket);
    }

    if (bucket < biosal_hash_table_buckets(self->current)) {
        return biosal_hash_table_state(self->current, bucket);
    }

    return biosal_hash_table_state(self->next, bucket - biosal_hash_table_buckets(self->current));
}

void *biosal_dynamic_hash_table_key(struct biosal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return biosal_hash_table_key(self->current, bucket);
    }

    if (biosal_dynamic_hash_table_resize(self)) {
        return biosal_hash_table_key(self->current, bucket);
    }

    if (bucket < biosal_hash_table_buckets(self->current)) {
        return biosal_hash_table_key(self->current, bucket);
    }

    return biosal_hash_table_key(self->next, bucket - biosal_hash_table_buckets(self->current));
}

void *biosal_dynamic_hash_table_value(struct biosal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return biosal_hash_table_value(self->current, bucket);
    }

    if (biosal_dynamic_hash_table_resize(self)) {
        return biosal_hash_table_value(self->current, bucket);
    }

    if (bucket < biosal_hash_table_buckets(self->current)) {
        return biosal_hash_table_value(self->current, bucket);
    }

    return biosal_hash_table_value(self->next, bucket - biosal_hash_table_buckets(self->current));
}

int biosal_dynamic_hash_table_pack_size(struct biosal_dynamic_hash_table *self)
{
    if (self->resize_in_progress) {
        biosal_dynamic_hash_table_finish_resizing(self);
    }

    return biosal_hash_table_pack_size(self->current);
}

int biosal_dynamic_hash_table_pack(struct biosal_dynamic_hash_table *self, void *buffer)
{
    if (self->resize_in_progress) {
        biosal_dynamic_hash_table_finish_resizing(self);
    }

    return biosal_hash_table_pack(self->current, buffer);
}

int biosal_dynamic_hash_table_unpack(struct biosal_dynamic_hash_table *self, void *buffer)
{
    biosal_dynamic_hash_table_reset(self);

    return biosal_hash_table_unpack(self->current, buffer);
}

void biosal_dynamic_hash_table_finish_resizing(struct biosal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return;
    }

#ifdef BIOSAL_DYNAMIC_HASH_TABLE_DEBUG_RESIZING
    if (biosal_hash_table_size(self->current) == 0) {
        biosal_tracer_print_stack_backtrace();
    }
    printf("DEBUG finish resizing current %" PRIu64 "/%" PRIu64 " next %" PRIu64 "/%" PRIu64 "\n",
                    biosal_hash_table_size(self->current), biosal_hash_table_buckets(self->current),
        biosal_hash_table_size(self->next), biosal_hash_table_buckets(self->next));
#endif

    while (self->resize_in_progress) {
        biosal_dynamic_hash_table_resize(self);
    }
}

int biosal_dynamic_hash_table_get_key_size(struct biosal_dynamic_hash_table *self)
{
    return biosal_hash_table_key_size(self->current);
}

int biosal_dynamic_hash_table_get_value_size(struct biosal_dynamic_hash_table *self)
{
    return biosal_hash_table_value_size(self->current);
}

void biosal_dynamic_hash_table_set_memory_pool(struct biosal_dynamic_hash_table *table,
                struct biosal_memory_pool *memory)
{
    if (table->current != NULL) {
        biosal_hash_table_set_memory_pool(table->current, memory);
    }

    if (table->next != NULL) {
        biosal_hash_table_set_memory_pool(table->next, memory);
    }
}

void biosal_dynamic_hash_table_disable_deletion_support(struct biosal_dynamic_hash_table *table)
{
    if (table->current != NULL) {
        biosal_hash_table_disable_deletion_support(table->current);
    }

    if (table->next != NULL) {
        biosal_hash_table_disable_deletion_support(table->next);
    }

}

void biosal_dynamic_hash_table_enable_deletion_support(struct biosal_dynamic_hash_table *table)
{
    if (table->current != NULL) {
        biosal_hash_table_enable_deletion_support(table->current);
    }

    if (table->next != NULL) {
        biosal_hash_table_enable_deletion_support(table->next);
    }
}

void biosal_dynamic_hash_table_set_current_size_estimate(struct biosal_dynamic_hash_table *table,
                double value)
{
    uint64_t current_size;
    double threshold;
    uint64_t size_estimate;
    uint64_t next_size;
    uint64_t required;
    int avoided_resizing_operations;
    uint64_t current_buckets;

    current_size = biosal_dynamic_hash_table_size(table);
    threshold = table->resize_load_threshold;
    current_buckets = biosal_hash_table_buckets(table->current);

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

void biosal_dynamic_hash_table_set_threshold(struct biosal_dynamic_hash_table *table, double threshold)
{
    table->resize_load_threshold = threshold;
}

int biosal_dynamic_hash_table_is_currently_resizing(struct biosal_dynamic_hash_table *table)
{
    return table->resize_in_progress;
}

void biosal_dynamic_hash_table_clear(struct biosal_dynamic_hash_table *self)
{
    biosal_dynamic_hash_table_finish_resizing(self);

    biosal_hash_table_clear(self->current);
}
