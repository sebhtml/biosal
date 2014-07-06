
#ifndef BSAL_HASH_TABLE_GROUP_H
#define BSAL_HASH_TABLE_GROUP_H

#define BSAL_HASH_TABLE_BUCKET_EMPTY 0x000000ff
#define BSAL_HASH_TABLE_BUCKET_OCCUPIED 0x0000ff00
#define BSAL_HASH_TABLE_BUCKET_DELETED 0x00ff0000

#include <stdint.h>

/* TODO: implement sparse hash method with bitmap */
struct bsal_hash_table_group {
    void *array;
    void *occupancy_bitmap;
    void *deletion_bitmap;
};

void bsal_hash_table_group_init(struct bsal_hash_table_group *group,
                uint64_t buckets_per_group, int key_size, int value_size);
void bsal_hash_table_group_destroy(struct bsal_hash_table_group *group);

int bsal_hash_table_group_buckets(struct bsal_hash_table_group *group);
void *bsal_hash_table_group_add(struct bsal_hash_table_group *group, uint64_t bucket,
                int key_size, int value_size);
void *bsal_hash_table_group_get(struct bsal_hash_table_group *group, uint64_t bucket,
                int key_size, int value_size);
void bsal_hash_table_group_delete(struct bsal_hash_table_group *group, uint64_t bucket);

int bsal_hash_table_group_state(struct bsal_hash_table_group *group, uint64_t bucket);

void *bsal_hash_table_group_key(struct bsal_hash_table_group *group, uint64_t bucket,
               int key_size, int value_size);
void *bsal_hash_table_group_value(struct bsal_hash_table_group *group, uint64_t bucket,
               int key_size, int value_size);

int bsal_hash_table_group_get_bit(void *bitmap, uint64_t bucket);
void bsal_hash_table_group_set_bit(void *bitmap, uint64_t bucket, int value);

int bsal_hash_table_group_pack_unpack(struct bsal_hash_table_group *self, void *buffer, int operation,
                uint64_t buckets_per_group, int key_size, int value_size);

#endif
