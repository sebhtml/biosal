
#include "dynamic_hash_table.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
#define BSAL_DYNAMIC_HASH_TABLE_DEBUG_ADD
*/

#define BSAL_DYNAMIC_HASH_TABLE_THRESHOLD 0.90

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

    /* if no resize is in progress, the load is verified
     */
    if (!self->resize_in_progress) {

        threshold = BSAL_DYNAMIC_HASH_TABLE_THRESHOLD;
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

    /* if the resizing finished, just return a recursive call
     */
    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_dynamic_hash_table_add(self, key);
    }

    /*
     * the current table has the key
     */
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

    /* recursive call if the resizing is finished
     */
    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_dynamic_hash_table_get(self, key);
    }

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

    if (bsal_dynamic_hash_table_resize(self)) {

        bsal_dynamic_hash_table_delete(self, key);
        return;
    }

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

    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_dynamic_hash_table_size(self);
    }

    return bsal_hash_table_size(self->current) + bsal_hash_table_size(self->next);
}

uint64_t bsal_dynamic_hash_table_buckets(struct bsal_dynamic_hash_table *self)
{
    if (!self->resize_in_progress) {
        return bsal_hash_table_buckets(self->current);
    }

    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_dynamic_hash_table_buckets(self);
    }

    return bsal_hash_table_buckets(self->current) + bsal_hash_table_buckets(self->next);
}

void bsal_dynamic_hash_table_start_resizing(struct bsal_dynamic_hash_table *self)
{
    uint64_t new_size;

    if (self->resize_in_progress) {
        return;
    }

    new_size = bsal_hash_table_buckets(self->current) * 2;

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
    printf("DEBUG bsal_dynamic_hash_table_start_resizing start resizing %d\n",
                    (int)new_size);
#endif

    self->resize_in_progress = 1;

    bsal_hash_table_init(self->next, new_size,
                    bsal_hash_table_key_size(self->current),
                    bsal_hash_table_value_size(self->current));

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
    count = 32;
    while (count-- && bsal_hash_table_iterator_has_next(&self->iterator)) {

        if (value_size == 0) {
            bsal_hash_table_iterator_next(&self->iterator, &key, NULL);
            bsal_hash_table_add(self->next, key);
        } else {
            bsal_hash_table_iterator_next(&self->iterator, &key, &value);
            new_value = bsal_hash_table_add(self->next, key);
            memcpy(new_value, value, value_size);
        }

        /* remove the old copy from current
         */
        bsal_hash_table_delete(self->current, key);
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

#ifdef BSAL_DYNAMIC_HASH_TABLE_DEBUG
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
        return bsal_dynamic_hash_table_state(self, bucket);
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
        return bsal_dynamic_hash_table_key(self, bucket);
    }

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

    if (bsal_dynamic_hash_table_resize(self)) {
        return bsal_dynamic_hash_table_value(self, bucket);
    }

    if (bucket < bsal_hash_table_buckets(self->current)) {
        return bsal_hash_table_value(self->current, bucket);
    }

    return bsal_hash_table_value(self->next, bucket);
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
    while (self->resize_in_progress) {
        bsal_dynamic_hash_table_resize(self);
    }
}


