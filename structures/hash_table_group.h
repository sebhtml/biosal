
#ifndef _BSAL_HASH_TABLE_GROUP_H
#define _BSAL_HASH_TABLE_GROUP_H

/* TODO: implement sparse hash method with bitmap */
struct bsal_hash_table_group {
    void *array;
    void *bitmap;
};

void bsal_hash_table_group_init(struct bsal_hash_table_group *group,
                int buckets_per_group, int key_size, int value_size);
void bsal_hash_table_group_destroy(struct bsal_hash_table_group *group);

void *bsal_hash_table_group_add(struct bsal_hash_table_group *group, int bucket,
                int key_size, int value_size);
void *bsal_hash_table_group_get(struct bsal_hash_table_group *group, int bucket,
                int key_size, int value_size);
void bsal_hash_table_group_delete(struct bsal_hash_table_group *group, int bucket);

int bsal_hash_table_group_state(struct bsal_hash_table_group *group, int bucket);
void *bsal_hash_table_group_key(struct bsal_hash_table_group *group, int bucket,
               int key_size, int value_size);

int bsal_hash_table_group_get_bit(struct bsal_hash_table_group *group, int bucket);
void bsal_hash_table_group_set_bit(struct bsal_hash_table_group *group, int bucket, int value1);

#endif
