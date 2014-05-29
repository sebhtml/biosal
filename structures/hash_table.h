
#ifndef _BSAL_HASH_TABLE_H
#define _BSAL_HASH_TABLE_H

#include "hash_table_group.h"

#include <stdint.h>

/**
 * features:
 *
 * - open addressing
 * - double-hashing
 *
 * possibly:
 *
 * - smart pointers
 * - incremental resizing
 */
struct bsal_hash_table {
    struct bsal_hash_table_group *groups;
    uint64_t elements;
    uint64_t buckets;

    int group_count;
    int buckets_per_group;
    int key_size;
    int value_size;
};

void bsal_hash_table_init(struct bsal_hash_table *table, uint64_t buckets,
                int key_size, int value_size);
void bsal_hash_table_destroy(struct bsal_hash_table *table);

void *bsal_hash_table_add(struct bsal_hash_table *table, void *key);
void *bsal_hash_table_get(struct bsal_hash_table *table, void *key);
void bsal_hash_table_delete(struct bsal_hash_table *table, void *key);

int bsal_hash_table_get_group(struct bsal_hash_table *table, uint64_t bucket);
int bsal_hash_table_get_group_bucket(struct bsal_hash_table *table, uint64_t bucket);

uint64_t bsal_hash_table_hash1(struct bsal_hash_table *table, void *key);
uint64_t bsal_hash_table_hash2(struct bsal_hash_table *table, void *key);
uint64_t bsal_hash_table_double_hash(struct bsal_hash_table *table, void *key, uint64_t stride);

int bsal_hash_table_find_bucket(struct bsal_hash_table *table, void *key, int *group, int *bucket_in_group);
uint64_t bsal_murmur_hash_64(const void *key, int len, unsigned int seed);

int bsal_hash_table_elements(struct bsal_hash_table *table);
int bsal_hash_table_buckets(struct bsal_hash_table *table);

#endif
