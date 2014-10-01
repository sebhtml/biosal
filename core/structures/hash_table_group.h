
#ifndef BIOSAL_HASH_TABLE_GROUP_H
#define BIOSAL_HASH_TABLE_GROUP_H

#define BIOSAL_HASH_TABLE_BUCKET_EMPTY 0x000000ff
#define BIOSAL_HASH_TABLE_BUCKET_OCCUPIED 0x0000ff00
#define BIOSAL_HASH_TABLE_BUCKET_DELETED 0x00ff0000

#include <stdint.h>

struct biosal_memory_pool;

/* TODO: implement sparse hash method with bitmap */

/**
 * This is a hash table group
 */
struct biosal_hash_table_group {
    void *array;
    void *occupancy_bitmap;
    void *deletion_bitmap;
};

void biosal_hash_table_group_init(struct biosal_hash_table_group *self,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct biosal_memory_pool *memory, int deletion_is_enabled);
void biosal_hash_table_group_destroy(struct biosal_hash_table_group *self,
                struct biosal_memory_pool *memory);

int biosal_hash_table_group_buckets(struct biosal_hash_table_group *self);
void *biosal_hash_table_group_add(struct biosal_hash_table_group *self, uint64_t bucket,
                int key_size, int value_size);
void *biosal_hash_table_group_get(struct biosal_hash_table_group *self, uint64_t bucket,
                int key_size, int value_size);
void biosal_hash_table_group_delete(struct biosal_hash_table_group *self, uint64_t bucket);

int biosal_hash_table_group_state(struct biosal_hash_table_group *self, uint64_t bucket);

void *biosal_hash_table_group_key(struct biosal_hash_table_group *self, uint64_t bucket,
               int key_size, int value_size);
void *biosal_hash_table_group_value(struct biosal_hash_table_group *self, uint64_t bucket,
               int key_size, int value_size);

int biosal_hash_table_group_get_bit(void *bitmap, uint64_t bucket);
void biosal_hash_table_group_set_bit(void *bitmap, uint64_t bucket, int value);

int biosal_hash_table_group_pack_unpack(struct biosal_hash_table_group *self, void *buffer, int operation,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct biosal_memory_pool *memory, int deletion_is_enabled);

#endif
