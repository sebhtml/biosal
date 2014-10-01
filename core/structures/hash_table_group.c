
#include "hash_table_group.h"

#include <core/system/packer.h>
#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <core/system/debugger.h>

#include <core/helpers/bitmap.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <inttypes.h>

/*#define BIOSAL_HASH_TABLE_GROUP_DEBUG */

void biosal_hash_table_group_init(struct biosal_hash_table_group *group,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct biosal_memory_pool *memory, int deletion_is_enabled)
{
    size_t bitmap_bytes;
    size_t array_bytes;

#ifdef BIOSAL_HASH_TABLE_GROUP_DEBUG
    printf("DEBUG init group\n");
#endif

    array_bytes = buckets_per_group * (key_size + value_size);
    bitmap_bytes = buckets_per_group / BIOSAL_BITS_PER_BYTE;

#ifdef BIOSAL_HASH_TABLE_GROUP_DEBUG
    printf("DEBUG buckets_per_group %" PRIu64 " key_size %d value_size %d\n",
                    buckets_per_group, key_size, value_size);
    printf("array_bytes %zu bitmap_bytes %zu\n", bitmap_bytes, array_bytes);
#endif

    /* use slab allocator */
    group->array = biosal_memory_pool_allocate(memory, array_bytes);
    group->occupancy_bitmap = biosal_memory_pool_allocate(memory, bitmap_bytes);

    group->deletion_bitmap = NULL;

    if (deletion_is_enabled) {
        group->deletion_bitmap = biosal_memory_pool_allocate(memory, bitmap_bytes);
    }

    /* mark all buckets as not occupied */
    memset(group->occupancy_bitmap, BIOSAL_BIT_ZERO, bitmap_bytes);

    if (group->deletion_bitmap != NULL) {
        memset(group->deletion_bitmap, BIOSAL_BIT_ZERO, bitmap_bytes);
    }
}

void biosal_hash_table_group_destroy(struct biosal_hash_table_group *group,
                struct biosal_memory_pool *memory)
{
    /* use slab allocator */
    biosal_memory_pool_free(memory, group->occupancy_bitmap);
    group->occupancy_bitmap = NULL;

    if (group->deletion_bitmap != NULL) {
        biosal_memory_pool_free(memory, group->deletion_bitmap);
        group->deletion_bitmap = NULL;
    }

    biosal_memory_pool_free(memory, group->array);
    group->array = NULL;
}

void biosal_hash_table_group_delete(struct biosal_hash_table_group *group, uint64_t bucket)
{
#ifdef BIOSAL_HASH_TABLE_GROUP_DEBUG
    printf("biosal_hash_table_group_delete setting occupancy to 0 and deleted to 1 for %i\n"
                    bucket);
#endif

    biosal_hash_table_group_set_bit(group->occupancy_bitmap, bucket,
                    BIOSAL_BIT_ZERO);

    if (group->deletion_bitmap != NULL) {
        biosal_hash_table_group_set_bit(group->deletion_bitmap, bucket,
                    BIOSAL_BIT_ONE);
    }
}

void *biosal_hash_table_group_add(struct biosal_hash_table_group *group,
                uint64_t bucket, int key_size, int value_size)
{
    biosal_hash_table_group_set_bit(group->occupancy_bitmap, bucket,
                    BIOSAL_BIT_ONE);

    if (group->deletion_bitmap != NULL) {
        biosal_hash_table_group_set_bit(group->deletion_bitmap, bucket,
                    BIOSAL_BIT_ZERO);
    }

    return biosal_hash_table_group_value(group, bucket, key_size, value_size);
}

void *biosal_hash_table_group_get(struct biosal_hash_table_group *group,
                uint64_t bucket, int key_size, int value_size)
{
    if (biosal_hash_table_group_state(group, bucket) ==
                    BIOSAL_HASH_TABLE_BUCKET_OCCUPIED) {

        return biosal_hash_table_group_value(group, bucket, key_size, value_size);
    }

    /* BIOSAL_HASH_TABLE_BUCKET_EMPTY and BIOSAL_HASH_TABLE_BUCKET_DELETED
     * are not occupied
     */
    return NULL;
}

/*
 * returns BIOSAL_HASH_TABLE_BUCKET_EMPTY or
 * BIOSAL_HASH_TABLE_BUCKET_OCCUPIED or
 * BIOSAL_HASH_TABLE_BUCKET_DELETED
 */
int biosal_hash_table_group_state(struct biosal_hash_table_group *group, uint64_t bucket)
{
    BIOSAL_DEBUGGER_ASSERT(group != NULL);

    if (biosal_hash_table_group_get_bit(group->occupancy_bitmap, bucket) == 1) {
        return BIOSAL_HASH_TABLE_BUCKET_OCCUPIED;
    }

    if (group->deletion_bitmap != NULL
                    && biosal_hash_table_group_get_bit(group->deletion_bitmap, bucket) == 1) {
        return BIOSAL_HASH_TABLE_BUCKET_DELETED;
    }

    return BIOSAL_HASH_TABLE_BUCKET_EMPTY;
}

void biosal_hash_table_group_set_bit(void *bitmap, uint64_t bucket, int value1)
{
    int unit;
    int bit;
    uint64_t bits;
    uint64_t value;
    uint64_t filter;

    value = value1;
    unit = bucket / BIOSAL_BITS_PER_BYTE;
    bit = bucket % BIOSAL_BITS_PER_BYTE;

    bits = (uint64_t)((char *)bitmap)[unit];

    if (value == BIOSAL_BIT_ONE){
        bits |= (value << bit);

    /* set bit to 0 */
    } else if (value == BIOSAL_BIT_ZERO) {
        filter = BIOSAL_BIT_ONE;
        filter <<= bit;
        filter =~ filter;
        bits &= filter;
    }

    (((char *)bitmap)[unit]) = bits;
}

int biosal_hash_table_group_get_bit(void *bitmap, uint64_t bucket)
{
    int unit;
    int bit;
    uint64_t bits;
    int bit_value;

    unit = bucket / BIOSAL_BITS_PER_BYTE;
    bit = bucket % BIOSAL_BITS_PER_BYTE;

    /*printf("biosal_hash_table_group_get_bit %p %i\n", group->bitmap, unit);*/

    bits = (uint64_t)(((char *)bitmap)[unit]);
    bit_value = (bits << (63 - bit)) >> 63;

    return bit_value;
}

void *biosal_hash_table_group_key(struct biosal_hash_table_group *group, uint64_t bucket,
                int key_size, int value_size)
{
    /* we assume that the key is stored first */
    return (char *)group->array + bucket * (key_size + value_size);
}

void *biosal_hash_table_group_value(struct biosal_hash_table_group *group, uint64_t bucket,
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
    return (char *)biosal_hash_table_group_key(group, bucket, key_size, value_size) + key_size;
}

int biosal_hash_table_group_pack_unpack(struct biosal_hash_table_group *self, void *buffer, int operation,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct biosal_memory_pool *memory, int deletion_is_enabled)
{

    struct biosal_packer packer;
    int offset;
    int bitmap_bytes;
    int array_bytes;

    if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {

            /*
             * The unpack is not required, this is done by the biosal_hash_table.
             *
        biosal_hash_table_group_init(self, buckets_per_group, key_size, value_size, memory,
                        deletion_is_enabled);
                        */
    }

    bitmap_bytes = buckets_per_group / BIOSAL_BITS_PER_BYTE;
    array_bytes = buckets_per_group * (key_size + value_size);

    biosal_packer_init(&packer, operation, buffer);

    biosal_packer_process(&packer, self->array, array_bytes);
    biosal_packer_process(&packer, self->occupancy_bitmap, bitmap_bytes);

    if (deletion_is_enabled) {
        biosal_packer_process(&packer, self->deletion_bitmap, bitmap_bytes);
    }

    offset = biosal_packer_get_byte_count(&packer);
    biosal_packer_destroy(&packer);

    return offset;
}


