
#include "hash_table.h"

#include <core/hash/hash.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <core/helpers/bitmap.h>
#include <core/system/packer.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <inttypes.h>

/* debugging options
 */
/*
#define CORE_HASH_TABLE_DEBUG
#define CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
#define CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG_STRIDE
*/

/*
 * Hash table operation codes.
 */
#define CORE_HASH_TABLE_OPERATION_ADD       0x000000ff
#define CORE_HASH_TABLE_OPERATION_GET       0x0000ff00
#define CORE_HASH_TABLE_OPERATION_DELETE    0x00ff0000

/*
 * Hash table flags.
 */
#define CORE_HASH_TABLE_FLAG_DEBUG              0
#define CORE_HASH_TABLE_FLAG_DELETION_SUPPORT   1

/*
 * Hash table results.
 */
#define CORE_HASH_TABLE_KEY_NOT_FOUND   0
#define CORE_HASH_TABLE_KEY_FOUND       1
#define CORE_HASH_TABLE_FULL            2

/*
 * Only use a single group
 */
#define CORE_HASH_TABLE_USE_ONE_GROUP

#define CORE_HASH_TABLE_MATCH 0

/*
 * functions for the implementation
 */
static int core_hash_table_get_group(struct core_hash_table *self, uint64_t bucket);
static int core_hash_table_get_group_bucket(struct core_hash_table *self, uint64_t bucket);

static uint64_t core_hash_table_hash1(struct core_hash_table *self, void *key);
static uint64_t core_hash_table_hash2(struct core_hash_table *self, void *key);
static uint64_t core_hash_table_double_hash(struct core_hash_table *self, uint64_t hash1,
                uint64_t hash2, uint64_t stride);
static int core_hash_table_find_bucket(struct core_hash_table *self, void *key,
                int *group, int *bucket_in_group, int operation, uint64_t *last_stride);

static int core_hash_table_pack_unpack(struct core_hash_table *self, void *buffer, int operation);
static void core_hash_table_start_groups(struct core_hash_table *self);
static uint64_t core_hash_table_hash(void *key, int key_size, unsigned int seed);

void core_hash_table_init(struct core_hash_table *table, uint64_t buckets,
                int key_size, int value_size)
{
    uint64_t buckets_per_group;
    uint64_t provided_buckets;

#ifdef CORE_HASH_TABLE_DEBUG_INIT
    printf("DEBUG core_hash_table_init\n");
#endif

    /*
     * Make sure the number of buckets is a multiple of 2
     */
    provided_buckets = buckets;
    buckets = 2;

    while (buckets < provided_buckets) {
        buckets *= 2;
    }


#ifdef CORE_HASH_TABLE_DEBUG_INIT
    printf("DEBUG %p core_hash_table_init buckets: %d key_size: %d value_size: %d\n",
                    (void *)table,
                    (int)buckets, key_size, value_size);
#endif

    CORE_BITMAP_CLEAR(table->flags);

    CORE_BITMAP_CLEAR_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG);

    /*
     * Google sparsehash uses 48. 64 is nice too
     */
    buckets_per_group = 64;

    while ((buckets & (buckets_per_group - 1)) != 0) {
        buckets++;
    }

#ifdef CORE_HASH_TABLE_USE_ONE_GROUP
    /*
     * Trick the code
     * CORE_HASH_TABLE_USE_ONE_GROUP makes the code faster, but is not compatible
     * with the idea of a sparse container.
     */
    buckets_per_group = buckets;
#endif

    table->buckets = buckets;

    /*
     * \see http://www.rohitab.com/discuss/topic/29723-modulus-with-bitwise-masks/
     */
    table->bucket_count_mask = table->buckets - 1;

#if 0
    printf("DEBUG buckets %" PRIu64 " bucket_count_mask %" PRIu64 "\n",
                    table->buckets, table->bucket_count_mask);
    printf("DEBUG buckets 0x%" PRIx64 " bucket_count_mask 0x%" PRIx64 "\n",
                    table->buckets, table->bucket_count_mask);
#endif

    table->buckets_per_group = buckets_per_group;
    table->group_bucket_count_mask = table->buckets_per_group - 1;

    table->key_size = key_size;
    table->value_size = value_size;

    table->elements = 0;
    table->group_count = (buckets / buckets_per_group);

    /* Use a lazy table
     */
    table->groups = NULL;
    core_hash_table_set_memory_pool(table, NULL);
    core_hash_table_enable_deletion_support(table);

    table->get_state = core_hash_table_group_state;
}

void core_hash_table_destroy(struct core_hash_table *table)
{
    int i;

    CORE_BITMAP_CLEAR_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG);

    if (table->groups != NULL) {
        for (i = 0; i < table->group_count; i++) {
            core_hash_table_group_destroy(table->groups + i,
                            table->memory);
        }

        core_memory_pool_free(table->memory, table->groups);
        table->groups = NULL;
    }

}

void core_hash_table_delete(struct core_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;
    int code;
    uint64_t last_stride;

    if (table->groups == NULL) {
        return;
    }

    if (!CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DELETION_SUPPORT)) {
        return;
    }

    code = core_hash_table_find_bucket(table, key, &group, &bucket_in_group,
                    CORE_HASH_TABLE_OPERATION_DELETE, &last_stride);

    if (code == CORE_HASH_TABLE_KEY_FOUND) {

#ifdef CORE_HASH_TABLE_DEBUG
        printf("core_hash_table_delete code CORE_HASH_TABLE_KEY_FOUND %i %i\n",
                        group, bucket_in_group);
#endif

        core_hash_table_group_delete(table->groups + group, bucket_in_group);
        table->elements--;
    }
}

static int core_hash_table_get_group(struct core_hash_table *table, uint64_t bucket)
{
    return bucket / table->buckets_per_group;
}

static int core_hash_table_get_group_bucket(struct core_hash_table *table, uint64_t bucket)
{
    return bucket & table->group_bucket_count_mask;
}

/*
 * The hash function can be changed here.
 */
uint64_t core_hash_table_hash(void *key, int key_size, unsigned int seed)
{
    if (key_size < 0) {
        printf("DEBUG ERROR key_size %d\n", key_size);
    }

    return core_hash_data_uint64_t(key, key_size, seed);
}

static uint64_t core_hash_table_hash1(struct core_hash_table *table, void *key)
{
    return core_hash_table_hash(key, table->key_size, 0x5cd902cb);
}

static uint64_t core_hash_table_hash2(struct core_hash_table *table, void *key)
{
    uint64_t hash2;

    hash2 = core_hash_table_hash(key, table->key_size, 0x80435418);

    /* the number of buckets and hash2 must be co-prime
     * the number of buckets is a power of 2
     * must be between 1 and M - 1
     */
    if (hash2 == 0) {
        hash2 = 1;
    }

    /* can not be a power of 2
     */
    if ((hash2 & 1) == 0) {
        hash2--;
    }

    return hash2;
}

static uint64_t core_hash_table_double_hash(struct core_hash_table *table, uint64_t hash1,
                uint64_t hash2, uint64_t stride)
{
    uint64_t result;

    result = hash1 + stride * hash2;

    return result & table->bucket_count_mask;
}

uint64_t core_hash_table_size(struct core_hash_table *table)
{
    return table->elements;
}

uint64_t core_hash_table_buckets(struct core_hash_table *table)
{
    return table->buckets;
}

void core_hash_table_toggle_debug(struct core_hash_table *table)
{
    if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
        CORE_BITMAP_CLEAR_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG);
        return;
    }

    CORE_BITMAP_SET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG);
}

int core_hash_table_state(struct core_hash_table *self, uint64_t bucket)
{
    int group;
    int bucket_in_group;
    struct core_hash_table_group *table_group;

    if (self->groups == NULL) {
        return CORE_HASH_TABLE_BUCKET_EMPTY;
    }

    if (bucket >= core_hash_table_buckets(self)) {
        return CORE_HASH_TABLE_BUCKET_EMPTY;
    }

#ifdef CORE_HASH_TABLE_USE_ONE_GROUP
    group = 0;
    bucket_in_group = core_hash_table_get_group_bucket(self, bucket);
#else
    group = core_hash_table_get_group(self, bucket);
    bucket_in_group = bucket;
#endif

    table_group = self->groups + group;

    return self->get_state(table_group, bucket_in_group);
}

void *core_hash_table_key(struct core_hash_table *self, uint64_t bucket)
{
    int group;
    int bucket_in_group;
    struct core_hash_table_group *table_group;

    if (self->groups == NULL) {
        return NULL;
    }

    if (bucket >= core_hash_table_buckets(self)) {
        return NULL;
    }

    group = core_hash_table_get_group(self, bucket);
    bucket_in_group = core_hash_table_get_group_bucket(self, bucket);

    table_group = self->groups + group;

    return core_hash_table_group_key(table_group, bucket_in_group,
                    self->key_size, self->value_size);
}

void *core_hash_table_value(struct core_hash_table *self, uint64_t bucket)
{
    int group;
    int bucket_in_group;
    struct core_hash_table_group *table_group;

    if (self->groups == NULL) {
        return NULL;
    }

    if (bucket >= core_hash_table_buckets(self)) {
        return NULL;
    }

    group = core_hash_table_get_group(self, bucket);
    bucket_in_group = core_hash_table_get_group_bucket(self, bucket);

    table_group = self->groups + group;

    return core_hash_table_group_value(table_group, bucket_in_group,
                    self->key_size, self->value_size);
}

int core_hash_table_value_size(struct core_hash_table *self)
{
    return self->value_size;
}

int core_hash_table_key_size(struct core_hash_table *self)
{
    return self->key_size;
}

int core_hash_table_pack_size(struct core_hash_table *self)
{
    return core_hash_table_pack_unpack(self, NULL, CORE_PACKER_OPERATION_PACK_SIZE);
}

int core_hash_table_pack(struct core_hash_table *self, void *buffer)
{
    return core_hash_table_pack_unpack(self, buffer, CORE_PACKER_OPERATION_PACK);
}

int core_hash_table_unpack(struct core_hash_table *self, void *buffer)
{
#ifdef CORE_HASH_TABLE_DEBUG
    printf("hash unpack\n");
#endif

    return core_hash_table_pack_unpack(self, buffer, CORE_PACKER_OPERATION_UNPACK);
}

static int core_hash_table_pack_unpack(struct core_hash_table *self, void *buffer, int operation)
{
    /* implement packing for the hash table
     */
    struct core_packer packer;
    int offset;
    int i;
    /*uint64_t elements;*/

#ifdef CORE_HASH_TABLE_DEBUG
    printf("hash pack/unpack %p\n", buffer);
#endif

    core_packer_init(&packer, operation, buffer);

    core_packer_process(&packer, &self->elements, sizeof(self->elements));
    core_packer_process(&packer, &self->buckets, sizeof(self->buckets));
    core_packer_process(&packer, &self->bucket_count_mask, sizeof(self->buckets));

    core_packer_process(&packer, &self->group_count, sizeof(self->group_count));
    core_packer_process(&packer, &self->buckets_per_group, sizeof(self->buckets_per_group));
    core_packer_process(&packer, &self->group_bucket_count_mask, sizeof(self->buckets_per_group));
    core_packer_process(&packer, &self->key_size, sizeof(self->key_size));
    core_packer_process(&packer, &self->value_size, sizeof(self->value_size));

    core_packer_process(&packer, &self->flags, sizeof(self->flags));

    offset = core_packer_get_byte_count(&packer);

#ifdef CORE_HASH_TABLE_DEBUG
    printf("before destroy\n");
#endif

    core_packer_destroy(&packer);

    if (operation == CORE_PACKER_OPERATION_UNPACK) {

#ifdef CORE_HASH_TABLE_DEBUG
        printf("hash init, buckets key_size value_size %d %d %d\n",
                         (int)self->buckets, self->key_size, self->value_size);
#endif
        /*
        elements = self->elements;
        self->elements = elements;
        */
#if 0
        self->groups = NULL;
        core_hash_table_set_memory_pool(self, NULL);
#endif

        core_hash_table_start_groups(self);
    } else {

        /* The code does not support an empty map with
         * no groups.
         */
        if (self->groups == NULL) {
            core_hash_table_start_groups(self);
        }
    }

    for (i = 0; i < self->group_count; i++) {

        offset += core_hash_table_group_pack_unpack(self->groups + i,
                        (char *)buffer + offset, operation,
                        self->buckets_per_group, self->key_size,
                        self->value_size, self->memory,
                        CORE_BITMAP_GET_BIT(self->flags, CORE_HASH_TABLE_FLAG_DELETION_SUPPORT));
    }

    return offset;
}

static void core_hash_table_start_groups(struct core_hash_table *table)
{
    int i;

    if (table->groups != NULL) {
        return;
    }

#ifdef CORE_HASH_TABLE_DEBUG_INIT
    printf("DEBUG core_hash_table_start_groups %p\n", (void *)table);
#endif

    table->groups = (struct core_hash_table_group *)
            core_memory_pool_allocate(table->memory,
                            table->group_count * sizeof(struct core_hash_table_group));

#ifdef CORE_HASH_TABLE_DEBUG_INIT
    printf("DEBUG core_hash_table_init group_count %d\n",
                    table->group_count);
#endif

    for (i = 0; i < table->group_count; i++) {
        core_hash_table_group_init(table->groups + i, table->buckets_per_group,
                        table->key_size, table->value_size, table->memory, CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DELETION_SUPPORT));
    }
}

void core_hash_table_set_memory_pool(struct core_hash_table *table, struct core_memory_pool *memory)
{
    table->memory = memory;
}

void core_hash_table_disable_deletion_support(struct core_hash_table *table)
{
    /*
     * Is no deletion can occur, then any key is always either in core_hash_table "current"
     * or in core_hash_table "next" or both. When it is in both, it means that the copy in "next"
     * is the primary copy and the other has no importance and will be garbage-collected anyway.
     *
     * Proof:
     *
     * Let x be a key and t be a core_map.
     *
     * t is currently resizing (each call to add / get / delete move 2 items from
     * "current" to "next").
     *
     * 1. key x is in core_hash_table "current"
     * 2. key x is moved from "current" to "next"
     * 3. key x is deleted from t ("next").
     * 4. key "x" is still in "current" and can be obtained with get().
     *
     * This situation can only arise if delete() is supported. Hence, core_dynamic_hash_table_resize()
     * must call core_hash_table_delete when moving data from "current" to "next" in the case
     * that the core_map (and core_dynamic_hash_table) supports deletion operations.
     *
     * Otherwise, this is fine.
     */

    CORE_BITMAP_CLEAR_BIT(table->flags, CORE_HASH_TABLE_FLAG_DELETION_SUPPORT);

    table->get_state = core_hash_table_group_state_no_deletion;
}

void core_hash_table_enable_deletion_support(struct core_hash_table *table)
{
    CORE_BITMAP_SET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DELETION_SUPPORT);

    table->get_state = core_hash_table_group_state;
}

int core_hash_table_deletion_support_is_enabled(struct core_hash_table *table)
{
    return CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DELETION_SUPPORT);
}

void *core_hash_table_add(struct core_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;
    void *bucket_key;
    int code;
    uint64_t last_stride;

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
    int query_value;
    int key_value;

    if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
        printf("DEBUG core_hash_table_add\n");
    }
#endif

    if (table->groups == NULL) {
        core_hash_table_start_groups(table);
    }

    code = core_hash_table_find_bucket(table, key, &group, &bucket_in_group,
                    CORE_HASH_TABLE_OPERATION_ADD, &last_stride);

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG_STRIDE

    printf("STRIDE load %" PRIu64 "/%" PRIu64 " stride %" PRIu64 "\n",
                    core_hash_table_size(table),
                    core_hash_table_buckets(table),
                    last_stride);
#endif

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
    if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
        printf("DEBUG core_hash_table_add group %d bucket_in_group %d code %d\n",
                        group, bucket_in_group, code);
    }
#endif

    if (code == CORE_HASH_TABLE_KEY_NOT_FOUND) {

#ifdef CORE_HASH_TABLE_DEBUG
        printf("core_hash_table_add code CORE_HASH_TABLE_KEY_NOT_FOUND"
                        "(group %i bucket %i)\n", group, bucket_in_group);
#endif

        /* install the key */
        bucket_key = core_hash_table_group_key(table->groups + group,
                        bucket_in_group, table->key_size, table->value_size);

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
            printf("DEBUG get key group %d bucket_in_group %d key_size %d value_size %d\n",
                            group, bucket_in_group, table->key_size, table->value_size);

            printf("DEBUG core_memory_copy %p %p %i\n", bucket_key, key, table->key_size);
        }
#endif

        core_memory_copy(bucket_key, key, table->key_size);

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {

            query_value = *(int *)key;
            key_value = *(int *)bucket_key;

            printf("DEBUG after copy query_value %d key_value %d bucket %p\n",
                            query_value, key_value, bucket_key);
        }
#endif

        table->elements++;

        return core_hash_table_group_add(table->groups + group, bucket_in_group,
                   table->key_size, table->value_size);

    } else if (code == CORE_HASH_TABLE_KEY_FOUND) {

#ifdef CORE_HASH_TABLE_DEBUG
        printf("core_hash_table_add code CORE_HASH_TABLE_KEY_FOUND"
                        "(group %i bucket %i)\n", group, bucket_in_group);
#endif

        return core_hash_table_group_get(table->groups + group, bucket_in_group,
                   table->key_size, table->value_size);

    } else if (code == CORE_HASH_TABLE_FULL) {

#ifdef CORE_HASH_TABLE_DEBUG
        printf("core_hash_table_add code CORE_HASH_TABLE_FULL\n");
#endif

        return NULL;
    }

    /* this statement is unreachable.
     */
    return NULL;
}

void *core_hash_table_get(struct core_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;
    int code;
    uint64_t last_stride;

    if (table->groups == NULL) {
        return NULL;
    }

    code = core_hash_table_find_bucket(table, key, &group, &bucket_in_group,
                    CORE_HASH_TABLE_OPERATION_GET, &last_stride);

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
    if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
        printf("DEBUG core_hash_table_get key %p group %d bucket_in_group %d code %d\n",
                        key, group, bucket_in_group, code);
    }
#endif

    /* core_hash_table_group_get would return NULL too,
     * but using this return code is cleaner */
    if (code == CORE_HASH_TABLE_KEY_NOT_FOUND) {
        return NULL;
    }

#ifdef CORE_HASH_TABLE_DEBUG
    printf("get %i %i code %i\n", group, bucket_in_group,
                    code);
#endif

    return core_hash_table_group_get(table->groups + group, bucket_in_group,
                    table->key_size, table->value_size);
}


/* this is the most important function for the hash table.
 * it finds a bucket with a key
 *
 * \param operation is one of these: CORE_HASH_TABLE_OPERATION_ADD,
 * CORE_HASH_TABLE_OPERATION_GET, CORE_HASH_TABLE_OPERATION_DELETE
 *
 * \return value is CORE_HASH_TABLE_KEY_FOUND or CORE_HASH_TABLE_KEY_NOT_FOUND or
 * CORE_HASH_TABLE_FULL
 */
static int core_hash_table_find_bucket(struct core_hash_table *table, void *key,
                int *group, int *bucket_in_group, int operation,
                uint64_t *last_stride)
{
    uint64_t bucket;
    uint64_t stride;
    int state;
    struct core_hash_table_group *hash_group;
    void *bucket_key;
    uint64_t hash1;
    uint64_t hash2;
    int local_group;
    int local_bucket_in_group;
    int operation_is_delete_or_get;
    int operation_is_add;

    operation_is_delete_or_get = 0;
    operation_is_add = 0;

    if (operation & CORE_HASH_TABLE_OPERATION_DELETE
                      ||
                  operation & CORE_HASH_TABLE_OPERATION_GET) {

        operation_is_delete_or_get = 1;

    } else if (operation & CORE_HASH_TABLE_OPERATION_ADD) {

        operation_is_add = 1;
    }

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
    int query_value;
    int key_value;

    if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
        printf("DEBUG core_hash_table_find_bucket\n");
    }
#endif

    stride = 0;

    hash1 = core_hash_table_hash1(table, key);
    hash2 = 0;

    /* Prevent the compiler from complaining about uninitialized
     * variables
     */
    local_group = 0;
    local_bucket_in_group = 0;

    /*
     * If only one hash group is used, it is not required
     * to compute it in the loop at all.
     */
#ifdef CORE_HASH_TABLE_USE_ONE_GROUP
    hash_group = table->groups + local_group;
#endif

    while (stride < table->buckets) {

        /* compute hash2 only on the second stride
         * It is not needed for stride # 0.
         * And strides #1, #2, #3 will used the same value.
         */
        if (stride == 1) {
            hash2 = core_hash_table_hash2(table, key);
        }

        bucket = core_hash_table_double_hash(table, hash1, hash2, stride);
        local_bucket_in_group = core_hash_table_get_group_bucket(table, bucket);

        /* Get the hash group if there is more than
         * one hash group.
         */
#ifndef CORE_HASH_TABLE_USE_ONE_GROUP
        local_group = core_hash_table_get_group(table, bucket);
        hash_group = table->groups + local_group;
#endif

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (table->groups == NULL) {
            printf("DEBUG %p Error groups is %p\n", (void *)table,
                            (void *)table->groups);
        }
#endif

        state = table->get_state(hash_group, local_bucket_in_group);

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
            printf("DEBUG stride %d bucket %d state %d\n", (int)stride, (int)bucket,
                            state);
        }
#endif

        /*
         * Case 1.
         * First case to test: check if the bucket is occupied by the
         * key.
         * we found a key, check if it matches the query.
         */
        if (state & CORE_HASH_TABLE_BUCKET_OCCUPIED) {

            /* the bucket is occupied, compare it with the key */
            bucket_key = core_hash_table_group_key(hash_group, local_bucket_in_group,
                        table->key_size, table->value_size);


            if (memcmp(bucket_key, key, table->key_size) ==
                CORE_HASH_TABLE_MATCH) {

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
                if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
                    printf("DEBUG state= OCCUPIED, match !\n");
                }
#endif
                *group = local_group;
                *bucket_in_group = local_bucket_in_group;
                *last_stride = stride;

                return CORE_HASH_TABLE_KEY_FOUND;
            }
        }

        /*
         * Case 2.
         *
         * We found an empty bucket to fulfil the procurement.
         */
        if (state & CORE_HASH_TABLE_BUCKET_EMPTY) {

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
            if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
                printf("DEBUG state= EMPTY\n");
            }
#endif

            *group = local_group;
            *bucket_in_group = local_bucket_in_group;
            *last_stride = stride;

            return CORE_HASH_TABLE_KEY_NOT_FOUND;
        }

        /*
         * Case 3.
         *
         * The bucket is deleted, and the operation is DELETE or GET.
         * In that case, this bucket is useless.
         *
         * Nothing to see here, it is deleted !
         * we only pick it up for CORE_HASH_TABLE_OPERATION_ADD
         * \see http://webdocs.cs.ualberta.ca/~holte/T26/open-addr.html
         */
        if (operation_is_delete_or_get
                        && (state & CORE_HASH_TABLE_BUCKET_DELETED)) {

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
            if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
                printf("DEBUG state= DELETE, op= DELETE or op= GET\n");
            }
#endif

            ++stride;
            continue;
        }

        /*
         * Case 4.
         *
         * A deleted bucket was found, it can be used to add
         * an item.
         */
        if (operation_is_add
                        && (state & CORE_HASH_TABLE_BUCKET_DELETED)) {

#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
            if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
                printf("DEBUG state= DELETED, op= ADD\n");
            }
#endif

            *group = local_group;
            *bucket_in_group = local_bucket_in_group;
            *last_stride = stride;

            return CORE_HASH_TABLE_KEY_NOT_FOUND;
        }

        /*
         * No match was found
         */
#ifdef CORE_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (CORE_BITMAP_GET_BIT(table->flags, CORE_HASH_TABLE_FLAG_DEBUG)) {
            printf("DEBUG state= OCCUPIED, no match, %d bytes\n",
                            table->key_size);
            query_value = *(int *)key;
            key_value = *(int *)bucket_key;

            printf("DEBUG query: %d, key: %d bucket_key %p\n", query_value, key_value,
                            bucket_key);
        }
#endif

        /* otherwise, continue the search
         */
        ++stride;
    }

    /* this statement will only be reached when the table is already full,
     * or if a key was not found and the table is full
     */

    *group = local_group;
    *bucket_in_group = local_bucket_in_group;
    *last_stride = stride;
    *last_stride = stride;

    if (operation_is_add) {
        return CORE_HASH_TABLE_FULL;
    }

    if (operation_is_delete_or_get) {
        return CORE_HASH_TABLE_KEY_NOT_FOUND;
    }

    /* This statement can not be reached, assuming the operation
     * is ADD, GET, or DELETE
     * UPDATE is implemented on top of this hash table, not directly inside
     * of it.
     */
    return CORE_HASH_TABLE_FULL;
}

void core_hash_table_clear(struct core_hash_table *self)
{
    int key_size;
    int value_size;
    struct core_memory_pool *pool;
    uint64_t buckets;

    key_size = self->key_size;
    value_size = self->value_size;
    pool = self->memory;
    buckets = self->buckets;

    core_hash_table_destroy(self);

    core_hash_table_init(self, key_size, value_size, buckets);

    if (pool != NULL) {
        core_hash_table_set_memory_pool(self, pool);
    }
}

struct core_memory_pool *core_hash_table_memory_pool(struct core_hash_table *self)
{
    return self->memory;
}
