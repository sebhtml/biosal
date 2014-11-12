
#ifndef CORE_HASH_TABLE_GROUP_H
#define CORE_HASH_TABLE_GROUP_H

#define CORE_HASH_TABLE_BUCKET_EMPTY 0x000000ff
#define CORE_HASH_TABLE_BUCKET_OCCUPIED 0x0000ff00
#define CORE_HASH_TABLE_BUCKET_DELETED 0x00ff0000

#include <stdint.h>

struct core_memory_pool;

/* TODO: implement sparse hash method with bitmap */

/*
 * Interleave keys and values. This provides better performance
 * because the only time where memory cache will be used is when
 * the key is the good one. In that case, the associated value will
 * be right after the key, and presumably in the same cache line.
 */
/*
*/
#define CORE_USE_INTERLEAVED_KEYS_AND_VALUES

/**
 * This is a hash table group
 */
struct core_hash_table_group {
#ifdef CORE_USE_INTERLEAVED_KEYS_AND_VALUES
    void *array;
#else
    /*
     * This is based on this patch:
     *
     * http://biosal.s3.amazonaws.com/patches/separate-keys-and-values-in-hash-table.c
     */
    /*
     * The keys and values are stored separately to increase
     * memory locality while doing searches in small hash tables.
     * For big hash tables, this will decrease performance.
     *
     * In practice, this probably does not change much since
     * memory accesses are random.
     */
    void *key_array;
    void *value_array;
#endif

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
int core_hash_table_group_state_no_deletion(struct core_hash_table_group *self, uint64_t bucket);

void *core_hash_table_group_key(struct core_hash_table_group *self, uint64_t bucket,
               int key_size, int value_size);
void *core_hash_table_group_value(struct core_hash_table_group *self, uint64_t bucket,
               int key_size, int value_size);

int core_hash_table_group_pack_unpack(struct core_hash_table_group *self, void *buffer, int operation,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct core_memory_pool *memory, int deletion_is_enabled);

#endif
