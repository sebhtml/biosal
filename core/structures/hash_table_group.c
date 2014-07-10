
#include "hash_table_group.h"

#include <core/system/packer.h>
#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <inttypes.h>

/*#define BSAL_HASH_TABLE_GROUP_DEBUG */

#define BSAL_BIT_ZERO 0
#define BSAL_BIT_ONE 1

#define BSAL_BITS_PER_BYTE 8

void bsal_hash_table_group_init(struct bsal_hash_table_group *group,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct bsal_memory_pool *memory, int deletion_is_enabled)
{
    size_t bitmap_bytes;
    size_t array_bytes;

    array_bytes = buckets_per_group * (key_size + value_size);
    bitmap_bytes = buckets_per_group / BSAL_BITS_PER_BYTE;

#ifdef BSAL_HASH_TABLE_GROUP_DEBUG
    printf("DEBUG buckets_per_group %" PRIu64 " key_size %d value_size %d\n",
                    buckets_per_group, key_size, value_size);
    printf("array_bytes %zu bitmap_bytes %zu\n", bitmap_bytes, array_bytes);
#endif

    /* use slab allocator */
    group->array = bsal_memory_pool_allocate(memory, array_bytes);
    group->occupancy_bitmap = bsal_memory_pool_allocate(memory, bitmap_bytes);

    group->deletion_bitmap = NULL;

    if (deletion_is_enabled) {
        group->deletion_bitmap = bsal_memory_pool_allocate(memory, bitmap_bytes);
    }

    /* mark all buckets as not occupied */
    memset(group->occupancy_bitmap, BSAL_BIT_ZERO, bitmap_bytes);

    if (deletion_is_enabled) {
        memset(group->deletion_bitmap, BSAL_BIT_ZERO, bitmap_bytes);
    }
}

void bsal_hash_table_group_destroy(struct bsal_hash_table_group *group,
                struct bsal_memory_pool *memory)
{
    /* use slab allocator */
    bsal_memory_pool_free(memory, group->occupancy_bitmap);
    group->occupancy_bitmap = NULL;

    if (group->deletion_bitmap != NULL) {
        bsal_memory_pool_free(memory, group->deletion_bitmap);
        group->deletion_bitmap = NULL;
    }

    bsal_memory_pool_free(memory, group->array);
    group->array = NULL;
}

void bsal_hash_table_group_delete(struct bsal_hash_table_group *group, uint64_t bucket)
{
#ifdef BSAL_HASH_TABLE_GROUP_DEBUG
    printf("bsal_hash_table_group_delete setting occupancy to 0 and deleted to 1 for %i\n"
                    bucket);
#endif

    bsal_hash_table_group_set_bit(group->occupancy_bitmap, bucket,
                    BSAL_BIT_ZERO);

    if (group->deletion_bitmap != NULL) {
        bsal_hash_table_group_set_bit(group->deletion_bitmap, bucket,
                    BSAL_BIT_ONE);
    }
}

void *bsal_hash_table_group_add(struct bsal_hash_table_group *group,
                uint64_t bucket, int key_size, int value_size)
{
    bsal_hash_table_group_set_bit(group->occupancy_bitmap, bucket,
                    BSAL_BIT_ONE);

    if (group->deletion_bitmap != NULL) {
        bsal_hash_table_group_set_bit(group->deletion_bitmap, bucket,
                    BSAL_BIT_ZERO);
    }

    return bsal_hash_table_group_value(group, bucket, key_size, value_size);
}

void *bsal_hash_table_group_get(struct bsal_hash_table_group *group,
                uint64_t bucket, int key_size, int value_size)
{
    if (bsal_hash_table_group_state(group, bucket) ==
                    BSAL_HASH_TABLE_BUCKET_OCCUPIED) {

        return bsal_hash_table_group_value(group, bucket, key_size, value_size);
    }

    /* BSAL_HASH_TABLE_BUCKET_EMPTY and BSAL_HASH_TABLE_BUCKET_DELETED
     * are not occupied
     */
    return NULL;
}

/*
 * returns BSAL_HASH_TABLE_BUCKET_EMPTY or
 * BSAL_HASH_TABLE_BUCKET_OCCUPIED or
 * BSAL_HASH_TABLE_BUCKET_DELETED
 */
int bsal_hash_table_group_state(struct bsal_hash_table_group *group, uint64_t bucket)
{
    if (bsal_hash_table_group_get_bit(group->occupancy_bitmap, bucket) == 1) {
        return BSAL_HASH_TABLE_BUCKET_OCCUPIED;
    }

    if (group->deletion_bitmap != NULL
                    && bsal_hash_table_group_get_bit(group->deletion_bitmap, bucket) == 1) {
        return BSAL_HASH_TABLE_BUCKET_DELETED;
    }

    return BSAL_HASH_TABLE_BUCKET_EMPTY;
}

void bsal_hash_table_group_set_bit(void *bitmap, uint64_t bucket, int value1)
{
    int unit;
    int bit;
    uint64_t bits;
    uint64_t value;

    value = value1;
    unit = bucket / BSAL_BITS_PER_BYTE;
    bit = bucket % BSAL_BITS_PER_BYTE;

    bits = (uint64_t)((char *)bitmap)[unit];

    if (value == BSAL_BIT_ONE){
        bits |= (value << bit);

        /* set bit to 0 */
    } else if (value == BSAL_BIT_ZERO) {
        uint64_t filter = BSAL_BIT_ONE;
        filter <<= bit;
        filter =~ filter;
        bits &= filter;
    }

    ((char *)bitmap)[unit] = bits;
}

int bsal_hash_table_group_get_bit(void *bitmap, uint64_t bucket)
{
    int unit;
    int bit;
    uint64_t bits;
    int bitValue;

    unit = bucket / BSAL_BITS_PER_BYTE;
    bit = bucket % BSAL_BITS_PER_BYTE;

    /*printf("bsal_hash_table_group_get_bit %p %i\n", group->bitmap, unit);*/

    bits = (uint64_t)((char *)bitmap)[unit];
    bitValue = (bits<<(63 - bit))>>63;

    return bitValue;
}

void *bsal_hash_table_group_key(struct bsal_hash_table_group *group, uint64_t bucket,
                int key_size, int value_size)
{
    /* we assume that the key is stored first */
    return (char *)group->array + bucket * (key_size + value_size);
}

void *bsal_hash_table_group_value(struct bsal_hash_table_group *group, uint64_t bucket,
                int key_size, int value_size)
{
    /*
     * It would be logical to return a NULL pointer
     * if value size is 0.
     * However, some use cases, like the set, require
     * a pointer even when value_size is 0.
     * Otherwise, it is impossible to verify if a key
     * is inside with the current interface
     *
     */
#if 0
    if (value_size == 0) {
        return NULL;
    }
#endif

    /* we assume that the key is stored first */
    return (char *)bsal_hash_table_group_key(group, bucket, key_size, value_size) + key_size;
}

int bsal_hash_table_group_pack_unpack(struct bsal_hash_table_group *self, void *buffer, int operation,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct bsal_memory_pool *memory, int deletion_is_enabled)
{

    struct bsal_packer packer;
    int offset;
    int bitmap_bytes;
    int array_bytes;

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        bsal_hash_table_group_init(self, buckets_per_group, key_size, value_size, memory,
                        deletion_is_enabled);
    }

    bitmap_bytes = buckets_per_group / BSAL_BITS_PER_BYTE;
    array_bytes = buckets_per_group * (key_size + value_size);

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, self->array, array_bytes);
    bsal_packer_work(&packer, self->occupancy_bitmap, bitmap_bytes);

    if (deletion_is_enabled) {
        bsal_packer_work(&packer, self->deletion_bitmap, bitmap_bytes);
    }

    offset = bsal_packer_worked_bytes(&packer);
    bsal_packer_destroy(&packer);

    return offset;
}


