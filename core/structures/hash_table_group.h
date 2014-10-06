
#ifndef CORE_HASH_TABLE_GROUP_H
#define CORE_HASH_TABLE_GROUP_H

#define CORE_HASH_TABLE_BUCKET_EMPTY 0x000000ff
#define CORE_HASH_TABLE_BUCKET_OCCUPIED 0x0000ff00
#define CORE_HASH_TABLE_BUCKET_DELETED 0x00ff0000

#include <stdint.h>

struct core_memory_pool;

/* TODO: implement sparse hash method with bitmap */

/**
 * This is a hash table group
 */
struct core_hash_table_group {
    void *array;
    void *occupancy_bitmap;
    void *deletion_bitmap;
};

void core_hash_table_group_init(struct core_hash_table_group *self,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct core_memory_pool *memory, int deletion_is_enabled);
void core_hash_table_group_destroy(struct core_hash_table_group *self,
                struct core_memory_pool *memory);

int core_hash_table_group_buckets(struct core_hash_table_group *self);
void *core_hash_table_group_add(struct core_hash_table_group *self, uint64_t bucket,
                int key_size, int value_size);
void *core_hash_table_group_get(struct core_hash_table_group *self, uint64_t bucket,
                int key_size, int value_size);
void core_hash_table_group_delete(struct core_hash_table_group *self, uint64_t bucket);

int core_hash_table_group_state(struct core_hash_table_group *self, uint64_t bucket);

void *core_hash_table_group_key(struct core_hash_table_group *self, uint64_t bucket,
               int key_size, int value_size);
void *core_hash_table_group_value(struct core_hash_table_group *self, uint64_t bucket,
               int key_size, int value_size);

int core_hash_table_group_pack_unpack(struct core_hash_table_group *self, void *buffer, int operation,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct core_memory_pool *memory, int deletion_is_enabled);

#endif
