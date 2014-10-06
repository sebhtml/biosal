
#ifndef CORE_DYNAMIC_HASH_TABLE_H
#define CORE_DYNAMIC_HASH_TABLE_H

#include "hash_table.h"
#include "hash_table_iterator.h"

struct core_memory_pool;

/*
 * This is a dynamic hash table with no size limit.
 * Note: Don't use this class. Instead, use core_map.
 */
struct core_dynamic_hash_table {
    struct core_hash_table table1;
    struct core_hash_table table2;

    /* information for resizing
     */
    struct core_hash_table_iterator iterator;
    struct core_hash_table *current;
    struct core_hash_table *next;

    int resize_in_progress;
    uint64_t resize_current_size;
    uint64_t resize_next_size;
    double resize_load_threshold;
};

void core_dynamic_hash_table_init(struct core_dynamic_hash_table *self, uint64_t buckets,
                int key_size, int value_size);
void core_dynamic_hash_table_destroy(struct core_dynamic_hash_table *self);

void *core_dynamic_hash_table_add(struct core_dynamic_hash_table *self, void *key);
void *core_dynamic_hash_table_get(struct core_dynamic_hash_table *self, void *key);
void core_dynamic_hash_table_delete(struct core_dynamic_hash_table *self, void *key);

uint64_t core_dynamic_hash_table_size(struct core_dynamic_hash_table *self);
uint64_t core_dynamic_hash_table_buckets(struct core_dynamic_hash_table *self);

void core_dynamic_hash_table_clear(struct core_dynamic_hash_table *self);

void *core_dynamic_hash_table_key(struct core_dynamic_hash_table *self, uint64_t bucket);
void *core_dynamic_hash_table_value(struct core_dynamic_hash_table *self, uint64_t bucket);

void core_dynamic_hash_table_finish_resizing(struct core_dynamic_hash_table *self);

int core_dynamic_hash_table_get_key_size(struct core_dynamic_hash_table *self);
int core_dynamic_hash_table_get_value_size(struct core_dynamic_hash_table *self);

int core_dynamic_hash_table_pack_size(struct core_dynamic_hash_table *self);
int core_dynamic_hash_table_state(struct core_dynamic_hash_table *self, uint64_t bucket);

int core_dynamic_hash_table_pack(struct core_dynamic_hash_table *self, void *buffer);
int core_dynamic_hash_table_unpack(struct core_dynamic_hash_table *self, void *buffer);
void core_dynamic_hash_table_set_memory_pool(struct core_dynamic_hash_table *self,
                struct core_memory_pool *memory);
struct core_memory_pool *core_dynamic_hash_table_memory_pool(struct core_dynamic_hash_table *self);

void core_dynamic_hash_table_disable_deletion_support(struct core_dynamic_hash_table *self);
void core_dynamic_hash_table_enable_deletion_support(struct core_dynamic_hash_table *self);
void core_dynamic_hash_table_set_current_size_estimate(struct core_dynamic_hash_table *self,
                double value);
void core_dynamic_hash_table_set_threshold(struct core_dynamic_hash_table *self, double threshold);
int core_dynamic_hash_table_is_currently_resizing(struct core_dynamic_hash_table *self);

#endif
