
#include "dynamic_hash_table.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
#define BSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
*/

void bsal_dynamic_hash_table_init(struct bsal_dynamic_hash_table *self, uint64_t buckets,
                int key_size, int value_size)
{
#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("DEBUG bsal_dynamic_hash_table_init \n");
#endif

    self->current = &self->table1;
    self->next = &self->table2;

    bsal_hash_table_init(self->current, buckets, key_size, value_size);

    self->resize_in_progress = 0;
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

void *bsal_dynamic_hash_table_add(struct bsal_dynamic_hash_table *self, void *key)
{
    float ratio;
    float threshold;

    if (!self->resize_in_progress) {

        threshold = 0.75;
        ratio = (0.0 + bsal_hash_table_size(self->current)) / bsal_hash_table_buckets(self->current);

        if (ratio < threshold) {
            return bsal_hash_table_add(self->current, key);
        } else {

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
            printf("DEBUG bsal_dynamic_hash_table_add start resizing\n");
#endif

            bsal_dynamic_hash_table_start_resizing(self);

            return bsal_dynamic_hash_table_add(self, key);
        }
    }

    bsal_dynamic_hash_table_resize(self);

    if (bsal_hash_table_get(self->current, key) != NULL) {
        return bsal_hash_table_get(self->current, key);
    }

    return bsal_hash_table_add(self->next, key);
}

void *bsal_dynamic_hash_table_get(struct bsal_dynamic_hash_table *self, void *key)
{
    void *bucket;

    if (!self->resize_in_progress) {
        return bsal_hash_table_get(self->current, key);
    }

    bsal_dynamic_hash_table_resize(self);

    bucket = bsal_hash_table_get(self->current, key);

    if (bucket != NULL) {
        return bucket;
    }

    return bsal_hash_table_get(self->next, key);
}

void bsal_dynamic_hash_table_delete(struct bsal_dynamic_hash_table *self, void *key)
{
    if (!self->resize_in_progress) {
        bsal_hash_table_delete(self->current, key);
        return;
    }

    bsal_dynamic_hash_table_resize(self);

    if (bsal_hash_table_get(self->current, key) != NULL) {
        bsal_hash_table_delete(self->current, key);
        return;
    }

    bsal_hash_table_delete(self->next, key);
}

uint64_t bsal_dynamic_hash_table_size(struct bsal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_size(self->current);
    }

    bsal_dynamic_hash_table_resize(self);

    return bsal_hash_table_size(self->current) + bsal_hash_table_size(self->next);
}

uint64_t bsal_dynamic_hash_table_buckets(struct bsal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_buckets(self->current);
    }

    bsal_dynamic_hash_table_resize(self);

    return bsal_hash_table_buckets(self->current) + bsal_hash_table_buckets(self->next);
}

void bsal_dynamic_hash_table_start_resizing(struct bsal_dynamic_hash_table *self)
{
    if (self->resize_in_progress) {
        return;
    }

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("DEBUG bsal_dynamic_hash_table_start_resizing start resizing\n");
#endif

    self->resize_in_progress = 1;

    bsal_hash_table_init(self->next, 2 * bsal_hash_table_buckets(self->current),
                    bsal_hash_table_key_size(self->current),
                    bsal_hash_table_value_size(self->current));

    bsal_hash_table_iterator_init(&self->iterator, self->current);
}

void bsal_dynamic_hash_table_resize(struct bsal_dynamic_hash_table *self)
{
    int count;
    struct bsal_hash_table *table;
    void *key;
    void *value;
    void *new_value;
    int value_size;

    if (!self->resize_in_progress) {
        return;
    }

    value_size = bsal_hash_table_value_size(self->current);

    /* transfer N elements in the next table
     */
    count = 32;
    while (count-- && bsal_hash_table_iterator_has_next(&self->iterator)) {

        bsal_hash_table_iterator_next(&self->iterator, &key, &value);

        new_value = bsal_hash_table_add(self->next, key);
        memcpy(new_value, value, value_size);

        /* remove the old copy from current
         */
        bsal_hash_table_delete(self->current, key);
    }

    if (!bsal_hash_table_iterator_has_next(&self->iterator)) {

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
        printf("DEBUG bsal_dynamic_hash_table_resize completed.\n");
#endif

        /* the transfer is finished, swap current and main
         */
        table = self->current;
        self->current = self->next;
        self->next = table;

        bsal_hash_table_iterator_destroy(&self->iterator);
        bsal_hash_table_destroy(self->next);

        self->resize_in_progress = 0;

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
        printf("DEBUG bsal_dynamic_hash_table_resize resizing is done\n");
#endif
    }
}

int bsal_dynamic_hash_table_state(struct bsal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_state(self->current, bucket);
    }

    bsal_dynamic_hash_table_resize(self);

    if (bucket < bsal_hash_table_buckets(self->current)) {
        return bsal_hash_table_state(self->current, bucket);
    }

    return bsal_hash_table_state(self->next, bucket);
}

void *bsal_dynamic_hash_table_key(struct bsal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_key(self->current, bucket);
    }

    bsal_dynamic_hash_table_resize(self);

    if (bucket < bsal_hash_table_buckets(self->current)) {
        return bsal_hash_table_key(self->current, bucket);
    }

    return bsal_hash_table_key(self->next, bucket);
}

void *bsal_dynamic_hash_table_value(struct bsal_dynamic_hash_table *self, uint64_t bucket)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_value(self->current, bucket);
    }

    bsal_dynamic_hash_table_resize(self);

    if (bucket < bsal_hash_table_buckets(self->current)) {
        return bsal_hash_table_value(self->current, bucket);
    }

    return bsal_hash_table_value(self->next, bucket);
}
