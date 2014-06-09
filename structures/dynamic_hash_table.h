
#ifndef _BSAL_DYNAMIC_HASH_TABLE_H
#define _BSAL_DYNAMIC_HASH_TABLE_H

#include "hash_table.h"
#include "hash_table_iterator.h"

struct bsal_dynamic_hash_table {
    struct bsal_hash_table table1;
    struct bsal_hash_table table2;
    struct bsal_hash_table_iterator iterator;
    struct bsal_hash_table *current;
    struct bsal_hash_table *next;

    int resize_in_progress;
};

void bsal_dynamic_hash_table_init(struct bsal_dynamic_hash_table *self, uint64_t buckets,
                int key_size, int value_size);
void bsal_dynamic_hash_table_destroy(struct bsal_dynamic_hash_table *self);

void *bsal_dynamic_hash_table_add(struct bsal_dynamic_hash_table *self, void *key);
void *bsal_dynamic_hash_table_get(struct bsal_dynamic_hash_table *self, void *key);
void bsal_dynamic_hash_table_delete(struct bsal_dynamic_hash_table *self, void *key);

uint64_t bsal_dynamic_hash_table_size(struct bsal_dynamic_hash_table *self);
uint64_t bsal_dynamic_hash_table_buckets(struct bsal_dynamic_hash_table *self);

void bsal_dynamic_hash_table_resize(struct bsal_dynamic_hash_table *self);
void bsal_dynamic_hash_table_start_resizing(struct bsal_dynamic_hash_table *self);

#endif
