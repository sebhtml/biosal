
#include "hash_table_group.h"

#include <stdlib.h>
#include <stdint.h>

/* TODO: use slab allocator */
void bsal_hash_table_group_init(struct bsal_hash_table_group *group,
                int buckets_per_group, int key_size, int value_size)
{
    /*printf("%i\n", buckets_per_group);*/
    group->bitmap = malloc(buckets_per_group / 8);
    group->array = malloc(buckets_per_group * (key_size + value_size));
}

void bsal_hash_table_group_destroy(struct bsal_hash_table_group *group)
{
    free(group->bitmap);
    group->bitmap = NULL;

    free(group->array);
    group->array = NULL;
}

void bsal_hash_table_group_delete(struct bsal_hash_table_group *group, int bucket)
{
    bsal_hash_table_group_set_bit(group, bucket, 0);
}

void *bsal_hash_table_group_add(struct bsal_hash_table_group *group,
                int bucket, int key_size, int value_size)
{
    bsal_hash_table_group_set_bit(group, bucket, 1);
    return (char *)group->array + bucket * (key_size + value_size);
}

void *bsal_hash_table_group_get(struct bsal_hash_table_group *group,
                int bucket, int key_size, int value_size)
{
    if (bsal_hash_table_group_state(group, bucket)) {
        return (char *)group->array + bucket * (key_size + value_size);
    }

    return NULL;
}

int bsal_hash_table_group_state(struct bsal_hash_table_group *group, int bucket)
{
    return bsal_hash_table_group_get_bit(group, bucket);
}

void bsal_hash_table_group_set_bit(struct bsal_hash_table_group *group, int bucket, int value1)
{
    int unit;
    int bit;
    uint64_t bits;
    uint64_t value;

    value = value1;
    unit = bucket / 8;
    bit = bucket % 8;

    bits = (uint64_t)((char *)group->bitmap)[unit];

    if (value == 1){
        bits |= (value << bit);

        /* set bit to 0 */
    } else if (value == 0) {
        uint64_t filter = 1;
        filter <<= bit;
        filter =~ filter;
        bits &= filter;
    }

    ((char *)group->bitmap)[unit] = bits;
}

int bsal_hash_table_group_get_bit(struct bsal_hash_table_group *group, int bucket)
{
    int unit;
    int bit;
    uint64_t bits;
    int bitValue;

    unit = bucket / 8;
    bit = bucket % 8;

    bits = (uint64_t)((char *)group->bitmap)[unit];
    bitValue = (bits<<(63 - bit))>>63;

    return bitValue;
}

void *bsal_hash_table_group_key(struct bsal_hash_table_group *group, int bucket,
                int key_size, int value_size)
{
    /* we assume that the key is stored first */
    return bsal_hash_table_group_get(group, bucket, key_size, value_size);
}
