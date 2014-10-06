
#ifndef CORE_HASH_TABLE_H
#define CORE_HASH_TABLE_H

#include "hash_table_group.h"

#include <stdint.h>

struct core_memory_pool;

/**
 * A hash table implementation.
 * Note: Don't use this class. Instead, use core_map.
 *
 * features:
 *
 * - [x] open addressing
 * - [x] double-hashing
 *
 * - [ ] sparsity (important)
 * - [ ] smart pointers (?)
 * - [x] incremental resizing (see struct core_dynamic_hash_table)
 *
 * for deletion, see http://webdocs.cs.ualberta.ca/~holte/T26/open-addr.html
 */
struct core_hash_table {
    struct core_hash_table_group *groups;
    uint64_t elements;
    uint64_t buckets;

    /*
     * \see http://www.rohitab.com/discuss/topic/29723-modulus-with-bitwise-masks/
     */
    uint64_t bucket_count_mask;
    uint64_t group_bucket_count_mask;

    int group_count;
    uint64_t buckets_per_group;
    int key_size;
    int value_size;

    uint32_t flags;

    struct core_memory_pool *memory;
};

/*
 * functions for user
 */
void core_hash_table_init(struct core_hash_table *self, uint64_t buckets,
                int key_size, int value_size);
void core_hash_table_destroy(struct core_hash_table *self);

void *core_hash_table_add(struct core_hash_table *self, void *key);
void *core_hash_table_get(struct core_hash_table *self, void *key);
void core_hash_table_delete(struct core_hash_table *self, void *key);

uint64_t core_hash_table_size(struct core_hash_table *self);
uint64_t core_hash_table_buckets(struct core_hash_table *self);
int core_hash_table_value_size(struct core_hash_table *self);
int core_hash_table_key_size(struct core_hash_table *self);
uint64_t core_hash_table_hash(void *key, int key_size, unsigned int seed);

int core_hash_table_state(struct core_hash_table *self, uint64_t bucket);
void *core_hash_table_key(struct core_hash_table *self, uint64_t bucket);
void *core_hash_table_value(struct core_hash_table *self, uint64_t bucket);

void core_hash_table_set_memory_pool(struct core_hash_table *self, struct core_memory_pool *memory);
struct core_memory_pool *core_hash_table_memory_pool(struct core_hash_table *self);

void core_hash_table_disable_deletion_support(struct core_hash_table *self);
void core_hash_table_enable_deletion_support(struct core_hash_table *self);
int core_hash_table_deletion_support_is_enabled(struct core_hash_table *self);

void core_hash_table_clear(struct core_hash_table *self);

int core_hash_table_pack_size(struct core_hash_table *self);
int core_hash_table_pack(struct core_hash_table *self, void *buffer);
int core_hash_table_unpack(struct core_hash_table *self, void *buffer);

void core_hash_table_toggle_debug(struct core_hash_table *self);

#endif
