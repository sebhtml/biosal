
#include "hash_table.h"

#include <stdlib.h>
#include <string.h>

void bsal_hash_table_init(struct bsal_hash_table *table, uint64_t buckets,
                int buckets_per_group, int key_size, int value_size)
{
    table->buckets = buckets;
    table->buckets_per_group = buckets_per_group;
    table->key_size = key_size;
    table->value_size = value_size;

    table->elements = 0;

    table->group_count = (buckets / buckets_per_group);

    table->groups = (struct bsal_hash_table_group *)
            malloc(table->group_count * sizeof(struct bsal_hash_table_group));
}

void bsal_hash_table_destroy(struct bsal_hash_table *table)
{
    free(table->groups);
    table->groups = NULL;
}

void *bsal_hash_table_add(struct bsal_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;

    bsal_hash_table_find_bucket(table, key, &group, &bucket_in_group);
    return bsal_hash_table_group_add(table->groups + group, bucket_in_group,
                   table->key_size, table->value_size);
}

void *bsal_hash_table_get(struct bsal_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;

    bsal_hash_table_find_bucket(table, key, &group, &bucket_in_group);
    return bsal_hash_table_group_get(table->groups + group, bucket_in_group,
                    table->key_size, table->value_size);
}

void bsal_hash_table_delete(struct bsal_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;

    bsal_hash_table_find_bucket(table, key, &group, &bucket_in_group);
    bsal_hash_table_group_delete(table->groups + group, bucket_in_group);
}

int bsal_hash_table_get_group(struct bsal_hash_table *table, uint64_t bucket)
{
    return bucket / table->buckets_per_group;
}

int bsal_hash_table_get_group_bucket(struct bsal_hash_table *table, uint64_t bucket)
{
    return bucket % table->buckets_per_group;
}

uint64_t bsal_hash_table_hash1(struct bsal_hash_table *table, void *key)
{
    return 0;
}

uint64_t bsal_hash_table_hash2(struct bsal_hash_table *table, void *key)
{
    return 0;
}

uint64_t bsal_hash_table_double_hash(struct bsal_hash_table *table, void *key, uint64_t stride)
{
    uint64_t hash1;
    uint64_t hash2;
    uint64_t result;

    hash2 = 0;
    hash1 = bsal_hash_table_hash1(table, key);

    if (stride > 0) {
        hash2 = bsal_hash_table_hash2(table, key);
    }

    result = (hash1 + stride * hash2) % table->buckets;

    return result;
}

void bsal_hash_table_find_bucket(struct bsal_hash_table *table, void *key, int *group, int *bucket_in_group)
{
    uint64_t stride;
    uint64_t bucket;
    int state;
    struct bsal_hash_table_group *hash_group;
    void *bucket_key;

    stride = 0;

    while (stride < table->buckets) {

        bucket = bsal_hash_table_double_hash(table, key, stride);
        *group = bsal_hash_table_get_group(table, bucket);
        *bucket_in_group = bsal_hash_table_get_group_bucket(table, bucket);
        hash_group = table->groups + *group;

        state = bsal_hash_table_group_state(hash_group, bucket);

        /* we found an empty bucket to fulful the procurement.
         */
        if (state == 0) {
            break;
        }

        bucket_key = bsal_hash_table_group_key(hash_group, bucket,
                        table->key_size, table->value_size);

        /*
         * we found the key
         */
        if (memcmp(bucket_key, key, table->key_size) == 0) {
            break;
        }

        stride++;
    }
}
