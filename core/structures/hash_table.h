
#ifndef BIOSAL_HASH_TABLE_H
#define BIOSAL_HASH_TABLE_H

#include "hash_table_group.h"

#include <stdint.h>

#define BIOSAL_HASH_TABLE_KEY_NOT_FOUND 0
#define BIOSAL_HASH_TABLE_KEY_FOUND 1
#define BIOSAL_HASH_TABLE_FULL 2

#define BIOSAL_HASH_TABLE_OPERATION_ADD 0x000000ff
#define BIOSAL_HASH_TABLE_OPERATION_GET 0x0000ff00
#define BIOSAL_HASH_TABLE_OPERATION_DELETE 0x00ff0000

#define BIOSAL_HASH_TABLE_MATCH 0

/* only use a single group
 */
#define BIOSAL_HASH_TABLE_USE_ONE_GROUP

struct biosal_memory_pool;

/**
 * A hash table implementation.
 * Note: Don't use this class. Instead, use biosal_map.
 *
 * features:
 *
 * - [x] open addressing
 * - [x] double-hashing
 *
 * - [ ] sparsity (important)
 * - [ ] smart pointers (?)
 * - [x] incremental resizing (see struct biosal_dynamic_hash_table)
 *
 * for deletion, see http://webdocs.cs.ualberta.ca/~holte/T26/open-addr.html
 */
struct biosal_hash_table {
    struct biosal_hash_table_group *groups;
    uint64_t elements;
    uint64_t buckets;

    int group_count;
    uint64_t buckets_per_group;
    int key_size;
    int value_size;

    int debug;

    struct biosal_memory_pool *memory;

    int deletion_is_enabled;
};

/*
 * functions for user
 */
void biosal_hash_table_init(struct biosal_hash_table *self, uint64_t buckets,
                int key_size, int value_size);
void biosal_hash_table_destroy(struct biosal_hash_table *self);

void *biosal_hash_table_add(struct biosal_hash_table *self, void *key);
void *biosal_hash_table_get(struct biosal_hash_table *self, void *key);
void biosal_hash_table_delete(struct biosal_hash_table *self, void *key);

uint64_t biosal_hash_table_size(struct biosal_hash_table *self);
uint64_t biosal_hash_table_buckets(struct biosal_hash_table *self);
int biosal_hash_table_value_size(struct biosal_hash_table *self);
int biosal_hash_table_key_size(struct biosal_hash_table *self);
uint64_t biosal_hash_table_hash(void *key, int key_size, unsigned int seed);

int biosal_hash_table_state(struct biosal_hash_table *self, uint64_t bucket);
void *biosal_hash_table_key(struct biosal_hash_table *self, uint64_t bucket);
void *biosal_hash_table_value(struct biosal_hash_table *self, uint64_t bucket);

/*
 * functions for the implementation
 */
int biosal_hash_table_get_group(struct biosal_hash_table *self, uint64_t bucket);
int biosal_hash_table_get_group_bucket(struct biosal_hash_table *self, uint64_t bucket);
int biosal_hash_table_state(struct biosal_hash_table *self, uint64_t bucket);

uint64_t biosal_hash_table_hash1(struct biosal_hash_table *self, void *key);
uint64_t biosal_hash_table_hash2(struct biosal_hash_table *self, void *key);
uint64_t biosal_hash_table_double_hash(struct biosal_hash_table *self, uint64_t hash1,
                uint64_t hash2, uint64_t stride);
int biosal_hash_table_find_bucket(struct biosal_hash_table *self, void *key,
                int *group, int *bucket_in_group, int operation, uint64_t *last_stride);
void biosal_hash_table_toggle_debug(struct biosal_hash_table *self);

int biosal_hash_table_pack_size(struct biosal_hash_table *self);
int biosal_hash_table_pack(struct biosal_hash_table *self, void *buffer);
int biosal_hash_table_unpack(struct biosal_hash_table *self, void *buffer);

int biosal_hash_table_pack_unpack(struct biosal_hash_table *self, void *buffer, int operation);
void biosal_hash_table_start_groups(struct biosal_hash_table *self);

void biosal_hash_table_set_memory_pool(struct biosal_hash_table *self, struct biosal_memory_pool *memory);
struct biosal_memory_pool *biosal_hash_table_memory_pool(struct biosal_hash_table *self);

void biosal_hash_table_disable_deletion_support(struct biosal_hash_table *self);
void biosal_hash_table_enable_deletion_support(struct biosal_hash_table *self);
int biosal_hash_table_deletion_support_is_enabled(struct biosal_hash_table *self);

void biosal_hash_table_clear(struct biosal_hash_table *self);

#endif
