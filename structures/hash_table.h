
#ifndef BSAL_HASH_TABLE_H
#define BSAL_HASH_TABLE_H

#include "hash_table_group.h"

#include <stdint.h>

/**
 * features:
 *
 * - [x] open addressing
 * - [x] double-hashing
 *
 * - [ ] sparsity (important)
 * - [ ] smart pointers (?)
 * - [ ] incremental resizing (?)
 *
 * for deletion, see http://webdocs.cs.ualberta.ca/~holte/T26/open-addr.html
 */
struct bsal_hash_table {
    struct bsal_hash_table_group *groups;
    uint64_t elements;
    uint64_t buckets;

    int group_count;
    uint64_t buckets_per_group;
    int key_size;
    int value_size;

    int debug;
};

/*
 * functions for user
 */
void bsal_hash_table_init(struct bsal_hash_table *table, uint64_t buckets,
                int key_size, int value_size);
void bsal_hash_table_destroy(struct bsal_hash_table *table);

void *bsal_hash_table_add(struct bsal_hash_table *table, void *key);
void *bsal_hash_table_get(struct bsal_hash_table *table, void *key);
void bsal_hash_table_delete(struct bsal_hash_table *table, void *key);

uint64_t bsal_hash_table_size(struct bsal_hash_table *table);
uint64_t bsal_hash_table_buckets(struct bsal_hash_table *table);
int bsal_hash_table_value_size(struct bsal_hash_table *table);
int bsal_hash_table_key_size(struct bsal_hash_table *table);
uint64_t bsal_hash_table_hash(void *key, int key_size, unsigned int seed);

int bsal_hash_table_state(struct bsal_hash_table *self, uint64_t bucket);
void *bsal_hash_table_key(struct bsal_hash_table *self, uint64_t bucket);
void *bsal_hash_table_value(struct bsal_hash_table *self, uint64_t bucket);

/*
 * functions for the implementation
 */
int bsal_hash_table_get_group(struct bsal_hash_table *table, uint64_t bucket);
int bsal_hash_table_get_group_bucket(struct bsal_hash_table *table, uint64_t bucket);
int bsal_hash_table_state(struct bsal_hash_table *table, uint64_t bucket);

uint64_t bsal_hash_table_hash1(struct bsal_hash_table *table, void *key);
uint64_t bsal_hash_table_hash2(struct bsal_hash_table *table, void *key);
uint64_t bsal_hash_table_double_hash(struct bsal_hash_table *table, uint64_t hash1,
                uint64_t hash2, uint64_t stride);
int bsal_hash_table_find_bucket(struct bsal_hash_table *table, void *key,
                int *group, int *bucket_in_group, int operation, uint64_t *last_stride);
void bsal_hash_table_toggle_debug(struct bsal_hash_table *table);

int bsal_hash_table_pack_size(struct bsal_hash_table *self);
int bsal_hash_table_pack(struct bsal_hash_table *self, void *buffer);
int bsal_hash_table_unpack(struct bsal_hash_table *self, void *buffer);

int bsal_hash_table_pack_unpack(struct bsal_hash_table *self, void *buffer, int operation);

#endif
