
#ifndef BIOSAL_DYNAMIC_HASH_TABLE_H
#define BIOSAL_DYNAMIC_HASH_TABLE_H

#include "hash_table.h"
#include "hash_table_iterator.h"

struct biosal_memory_pool;

/*
 * This is a dynamic hash table with no size limit.
 * Note: Don't use this class. Instead, use biosal_map.
 */
struct biosal_dynamic_hash_table {
    struct biosal_hash_table table1;
    struct biosal_hash_table table2;

    /* information for resizing
     */
    struct biosal_hash_table_iterator iterator;
    struct biosal_hash_table *current;
    struct biosal_hash_table *next;

    int resize_in_progress;
    uint64_t resize_current_size;
    uint64_t resize_next_size;
    double resize_load_threshold;
};

void biosal_dynamic_hash_table_init(struct biosal_dynamic_hash_table *self, uint64_t buckets,
                int key_size, int value_size);
void biosal_dynamic_hash_table_destroy(struct biosal_dynamic_hash_table *self);

void *biosal_dynamic_hash_table_add(struct biosal_dynamic_hash_table *self, void *key);
void *biosal_dynamic_hash_table_get(struct biosal_dynamic_hash_table *self, void *key);
void biosal_dynamic_hash_table_delete(struct biosal_dynamic_hash_table *self, void *key);

uint64_t biosal_dynamic_hash_table_size(struct biosal_dynamic_hash_table *self);
uint64_t biosal_dynamic_hash_table_buckets(struct biosal_dynamic_hash_table *self);

/**
 * \return 1 if resizing was completed, 0 otherwise
 */
int biosal_dynamic_hash_table_resize(struct biosal_dynamic_hash_table *self);
void biosal_dynamic_hash_table_start_resizing(struct biosal_dynamic_hash_table *self);

int biosal_dynamic_hash_table_state(struct biosal_dynamic_hash_table *self, uint64_t bucket);
void *biosal_dynamic_hash_table_key(struct biosal_dynamic_hash_table *self, uint64_t bucket);
void *biosal_dynamic_hash_table_value(struct biosal_dynamic_hash_table *self, uint64_t bucket);

int biosal_dynamic_hash_table_pack_size(struct biosal_dynamic_hash_table *self);
int biosal_dynamic_hash_table_pack(struct biosal_dynamic_hash_table *self, void *buffer);
int biosal_dynamic_hash_table_unpack(struct biosal_dynamic_hash_table *self, void *buffer);

void biosal_dynamic_hash_table_finish_resizing(struct biosal_dynamic_hash_table *self);
void biosal_dynamic_hash_table_reset(struct biosal_dynamic_hash_table *self);

int biosal_dynamic_hash_table_get_key_size(struct biosal_dynamic_hash_table *self);
int biosal_dynamic_hash_table_get_value_size(struct biosal_dynamic_hash_table *self);

void biosal_dynamic_hash_table_set_memory_pool(struct biosal_dynamic_hash_table *self,
                struct biosal_memory_pool *memory);

void biosal_dynamic_hash_table_disable_deletion_support(struct biosal_dynamic_hash_table *self);
void biosal_dynamic_hash_table_enable_deletion_support(struct biosal_dynamic_hash_table *self);
void biosal_dynamic_hash_table_set_current_size_estimate(struct biosal_dynamic_hash_table *self,
                double value);
void biosal_dynamic_hash_table_set_threshold(struct biosal_dynamic_hash_table *self, double threshold);
int biosal_dynamic_hash_table_is_currently_resizing(struct biosal_dynamic_hash_table *self);

void biosal_dynamic_hash_table_clear(struct biosal_dynamic_hash_table *self);

#endif
