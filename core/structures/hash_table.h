
#ifndef CORE_HASH_TABLE_H
#define CORE_HASH_TABLE_H

#include "hash_table_group.h"

#include <stdint.h>

#define CORE_HASH_TABLE_KEY_NOT_FOUND 0
#define CORE_HASH_TABLE_KEY_FOUND 1
#define CORE_HASH_TABLE_FULL 2

#define CORE_HASH_TABLE_OPERATION_ADD 0x000000ff
#define CORE_HASH_TABLE_OPERATION_GET 0x0000ff00
#define CORE_HASH_TABLE_OPERATION_DELETE 0x00ff0000

#define CORE_HASH_TABLE_MATCH 0

/*
 * Only use a single group
 */
#define CORE_HASH_TABLE_USE_ONE_GROUP

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

    int debug;

    struct core_memory_pool *memory;

    int deletion_is_enabled;
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

/*
 * functions for the implementation
 */
int core_hash_table_get_group(struct core_hash_table *self, uint64_t bucket);
int core_hash_table_get_group_bucket(struct core_hash_table *self, uint64_t bucket);
int core_hash_table_state(struct core_hash_table *self, uint64_t bucket);

uint64_t core_hash_table_hash1(struct core_hash_table *self, void *key);
uint64_t core_hash_table_hash2(struct core_hash_table *self, void *key);
uint64_t core_hash_table_double_hash(struct core_hash_table *self, uint64_t hash1,
                uint64_t hash2, uint64_t stride);
int core_hash_table_find_bucket(struct core_hash_table *self, void *key,
                int *group, int *bucket_in_group, int operation, uint64_t *last_stride);
void core_hash_table_toggle_debug(struct core_hash_table *self);

int core_hash_table_pack_size(struct core_hash_table *self);
int core_hash_table_pack(struct core_hash_table *self, void *buffer);
int core_hash_table_unpack(struct core_hash_table *self, void *buffer);

int core_hash_table_pack_unpack(struct core_hash_table *self, void *buffer, int operation);
void core_hash_table_start_groups(struct core_hash_table *self);

void core_hash_table_set_memory_pool(struct core_hash_table *self, struct core_memory_pool *memory);
struct core_memory_pool *core_hash_table_memory_pool(struct core_hash_table *self);

void core_hash_table_disable_deletion_support(struct core_hash_table *self);
void core_hash_table_enable_deletion_support(struct core_hash_table *self);
int core_hash_table_deletion_support_is_enabled(struct core_hash_table *self);

void core_hash_table_clear(struct core_hash_table *self);

#endif
