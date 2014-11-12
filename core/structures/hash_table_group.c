
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

/*#define CORE_HASH_TABLE_GROUP_DEBUG */

static int core_hash_table_group_get_bit(void *bitmap, uint64_t bucket);
static void core_hash_table_group_set_bit(void *bitmap, uint64_t bucket, int value);

void core_hash_table_group_init(struct core_hash_table_group *group,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct core_memory_pool *memory, int deletion_is_enabled)
{
    size_t bitmap_bytes;
    size_t array_bytes;

#ifdef CORE_HASH_TABLE_GROUP_DEBUG
    printf("DEBUG init group\n");
#endif

    /*
     * Allocate memory for data.
     * With CORE_USE_INTERLEAVED_KEYS_AND_VALUES, keys and values are
     * interleaved.
     */
#ifdef CORE_USE_INTERLEAVED_KEYS_AND_VALUES
    array_bytes = buckets_per_group * (key_size + value_size);

    group->array = core_memory_pool_allocate(memory, array_bytes);
#else
    array_bytes = buckets_per_group * key_size;

#ifdef CORE_HASH_TABLE_GROUP_DEBUG
    printf("DEBUG buckets_per_group %" PRIu64 " key_size %d value_size %d\n",
                    buckets_per_group, key_size, value_size);
    printf("array_bytes %zu bitmap_bytes %zu\n", bitmap_bytes, array_bytes);
#endif

    /* use slab allocator */
    group->key_array = core_memory_pool_allocate(memory, array_bytes);

    group->value_array = NULL;

    if (value_size) {
        array_bytes = buckets_per_group * value_size;
        group->value_array = core_memory_pool_allocate(memory, array_bytes);
    }
#endif

    /*
     * Allocate memory for bitmaps.
     */
    bitmap_bytes = buckets_per_group / CORE_BITS_PER_BYTE;

    group->occupancy_bitmap = core_memory_pool_allocate(memory, bitmap_bytes);

    group->deletion_bitmap = NULL;

    if (deletion_is_enabled) {
        group->deletion_bitmap = core_memory_pool_allocate(memory, bitmap_bytes);
    }

    /* mark all buckets as not occupied */
    memset(group->occupancy_bitmap, CORE_BIT_ZERO, bitmap_bytes);

    if (group->deletion_bitmap != NULL) {
        memset(group->deletion_bitmap, CORE_BIT_ZERO, bitmap_bytes);
    }
}

void core_hash_table_group_destroy(struct core_hash_table_group *group,
                struct core_memory_pool *memory)
{
    /* use slab allocator */
    core_memory_pool_free(memory, group->occupancy_bitmap);
    group->occupancy_bitmap = NULL;

    if (group->deletion_bitmap != NULL) {
        core_memory_pool_free(memory, group->deletion_bitmap);
        group->deletion_bitmap = NULL;
    }

#ifdef CORE_USE_INTERLEAVED_KEYS_AND_VALUES
    if (group->array != NULL) {
        core_memory_pool_free(memory, group->array);
        group->array = NULL;
    }
#else
    if (group->key_array != NULL) {
        core_memory_pool_free(memory, group->key_array);
        group->key_array = NULL;
    }

    if (group->value_array != NULL) {
        core_memory_pool_free(memory, group->value_array);
        group->value_array = NULL;
    }
#endif
}

void core_hash_table_group_delete(struct core_hash_table_group *group, uint64_t bucket)
{
#ifdef CORE_HASH_TABLE_GROUP_DEBUG
    printf("core_hash_table_group_delete setting occupancy to 0 and deleted to 1 for %i\n"
                    bucket);
#endif

    core_hash_table_group_set_bit(group->occupancy_bitmap, bucket,
                    CORE_BIT_ZERO);

    if (group->deletion_bitmap != NULL) {
        core_hash_table_group_set_bit(group->deletion_bitmap, bucket,
                    CORE_BIT_ONE);
    }
}

void *core_hash_table_group_add(struct core_hash_table_group *group,
                uint64_t bucket, int key_size, int value_size)
{
    core_hash_table_group_set_bit(group->occupancy_bitmap, bucket,
                    CORE_BIT_ONE);

    if (group->deletion_bitmap != NULL) {
        core_hash_table_group_set_bit(group->deletion_bitmap, bucket,
                    CORE_BIT_ZERO);
    }

    return core_hash_table_group_value(group, bucket, key_size, value_size);
}

void *core_hash_table_group_get(struct core_hash_table_group *group,
                uint64_t bucket, int key_size, int value_size)
{
    /*
     * If check_state is 0, just return the bucket without checking the
     * state.
     */
    if (core_hash_table_group_state(group, bucket) ==
                    CORE_HASH_TABLE_BUCKET_OCCUPIED) {

        return core_hash_table_group_value(group, bucket, key_size, value_size);
    }

    /* CORE_HASH_TABLE_BUCKET_EMPTY and CORE_HASH_TABLE_BUCKET_DELETED
     * are not occupied
     */
    return NULL;
}

/*
 * returns CORE_HASH_TABLE_BUCKET_EMPTY or
 * CORE_HASH_TABLE_BUCKET_OCCUPIED or
 * CORE_HASH_TABLE_BUCKET_DELETED
 */
int core_hash_table_group_state(struct core_hash_table_group *group, uint64_t bucket)
{
    CORE_DEBUGGER_ASSERT(group != NULL);

    if (core_hash_table_group_get_bit(group->occupancy_bitmap, bucket)) {
        return CORE_HASH_TABLE_BUCKET_OCCUPIED;
    }

    if (group->deletion_bitmap != NULL
                    && core_hash_table_group_get_bit(group->deletion_bitmap, bucket)) {
        return CORE_HASH_TABLE_BUCKET_DELETED;
    }

    return CORE_HASH_TABLE_BUCKET_EMPTY;
}

int core_hash_table_group_state_no_deletion(struct core_hash_table_group *group, uint64_t bucket)
{
    CORE_DEBUGGER_ASSERT(group != NULL);

    if (core_hash_table_group_get_bit(group->occupancy_bitmap, bucket)) {
        return CORE_HASH_TABLE_BUCKET_OCCUPIED;
    }

    return CORE_HASH_TABLE_BUCKET_EMPTY;
}

static void core_hash_table_group_set_bit(void *bitmap, uint64_t bucket, int value1)
{
    int unit;
    int bit;
    uint64_t bits;
    uint64_t value;
    uint64_t filter;

    value = value1;
#if 0
    unit = bucket / CORE_BITS_PER_BYTE;
#endif
    /*
     * CORE_BITS_PER_BYTE = 2^3 = 8
     */
    unit = bucket >> 3;
    bit = bucket & (CORE_BITS_PER_BYTE - 1);

    bits = (uint64_t)((char *)bitmap)[unit];

    if (value == CORE_BIT_ONE){
        bits |= (value << bit);

    /* set bit to 0 */
    } else if (value == CORE_BIT_ZERO) {
        filter = (1 << bit);
        filter =~ filter;
        bits &= filter;
    }

    (((char *)bitmap)[unit]) = bits;
}

static int core_hash_table_group_get_bit(void *bitmap, uint64_t bucket)
{
    int unit;
    int bit;
    uint64_t bits;
    int bit_value;

    /*
     * This is equivalent to this:
     *
     *      unit = bucket / CORE_BITS_PER_BYTE;
     */
    unit = bucket >> 3;
    bit = bucket & (CORE_BITS_PER_BYTE - 1);

    /*printf("core_hash_table_group_get_bit %p %i\n", group->bitmap, unit);*/

    bits = (uint64_t)(((char *)bitmap)[unit]);
    /*
    bit_value = (bits << (63 - bit)) >> 63;
    */
    bit_value = bits & (1 << bit);

    return bit_value;
}

void *core_hash_table_group_key(struct core_hash_table_group *group, uint64_t bucket,
                int key_size, int value_size)
{
    int stride;
    void *array;

#ifdef CORE_USE_INTERLEAVED_KEYS_AND_VALUES
    array = group->array;
    stride = key_size + value_size;
#else
    array = group->key_array;
    stride = key_size;
#endif

    /* we assume that the key is stored first */
    return (char *)array + bucket * stride;
}

void *core_hash_table_group_value(struct core_hash_table_group *group, uint64_t bucket,
                int key_size, int value_size)
{
    void *array;
    int stride;
    int offset;

    /*
     * It would be logical to return a NULL pointer
     * if value size is 0.
     * However, some use cases, like the set, require
     * a pointer even when value_size is 0.
     * Otherwise, it is impossible to verify if a key
     * is inside with the current interface
     *
     * TODO The core_set code should be fixed since this does not make much sense.
     */
    if (value_size == 0)
        return core_hash_table_group_key(group, bucket, key_size, value_size);

#if 0
    if (value_size == 0) {
        return NULL;
    }
#endif

#ifdef CORE_USE_INTERLEAVED_KEYS_AND_VALUES
    offset = key_size;
    array = group->array;
    stride = key_size + value_size;
#else
    offset = 0;
    array = group->value_array;
    stride = value_size;
#endif

    /* we assume that the key is stored first */

    return (char *)array + bucket * stride + offset;
}

int core_hash_table_group_pack_unpack(struct core_hash_table_group *self, void *buffer, int operation,
                uint64_t buckets_per_group, int key_size, int value_size,
                struct core_memory_pool *memory, int deletion_is_enabled)
{

    struct core_packer packer;
    int offset;
    int bitmap_bytes;
    int array_bytes;

    if (operation == CORE_PACKER_OPERATION_UNPACK) {

            /*
             * The unpack is not required, this is done by the core_hash_table.
             *
        core_hash_table_group_init(self, buckets_per_group, key_size, value_size, memory,
                        deletion_is_enabled);
                        */
    }

    bitmap_bytes = buckets_per_group / CORE_BITS_PER_BYTE;

    core_packer_init(&packer, operation, buffer);

#ifdef CORE_USE_INTERLEAVED_KEYS_AND_VALUES
    array_bytes = buckets_per_group * (key_size + value_size);
    if (self->array != NULL)
        core_packer_process(&packer, self->array, array_bytes);
#else
    array_bytes = buckets_per_group * key_size;
    if (self->key_array != NULL)
        core_packer_process(&packer, self->key_array, array_bytes);

    array_bytes = buckets_per_group * value_size;
    if (self->value_array != NULL)
        core_packer_process(&packer, self->value_array, array_bytes);
#endif

    core_packer_process(&packer, self->occupancy_bitmap, bitmap_bytes);

    if (deletion_is_enabled) {
        core_packer_process(&packer, self->deletion_bitmap, bitmap_bytes);
    }

    offset = core_packer_get_byte_count(&packer);
    core_packer_destroy(&packer);

    return offset;
}


