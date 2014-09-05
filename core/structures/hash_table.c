
#include "hash_table.h"

#include <core/hash/murmur_hash_2_64_a.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <core/system/packer.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <inttypes.h>

/* debugging options
 */
/*
#define BSAL_HASH_TABLE_DEBUG
#define BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
#define BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG_STRIDE
*/

void bsal_hash_table_init(struct bsal_hash_table *table, uint64_t buckets,
                int key_size, int value_size)
{
    uint64_t buckets_per_group;
    uint64_t provided_buckets;

#ifdef BSAL_HASH_TABLE_DEBUG_INIT
    printf("DEBUG bsal_hash_table_init\n");
#endif

    /*
     * Make sure the number of buckets is a multiple of 2
     */
    provided_buckets = buckets;
    buckets = 2;

    while (buckets < provided_buckets) {
        buckets *= 2;
    }


#ifdef BSAL_HASH_TABLE_DEBUG_INIT
    printf("DEBUG %p bsal_hash_table_init buckets: %d key_size: %d value_size: %d\n",
                    (void *)table,
                    (int)buckets, key_size, value_size);
#endif

    table->debug = 0;

    /* google sparsehash uses 48. 64 is nice too */
    buckets_per_group = 64;

    while (buckets % buckets_per_group != 0) {
        buckets++;
    }

#ifdef BSAL_HASH_TABLE_USE_ONE_GROUP
    /* trick the code
     */
    buckets_per_group = buckets;
#endif

    table->buckets = buckets;
    table->buckets_per_group = buckets_per_group;
    table->key_size = key_size;
    table->value_size = value_size;

    table->elements = 0;
    table->group_count = (buckets / buckets_per_group);

    /* Use a lazy table
     */
    table->groups = NULL;
    bsal_hash_table_set_memory_pool(table, NULL);
    bsal_hash_table_enable_deletion_support(table);
}

void bsal_hash_table_destroy(struct bsal_hash_table *table)
{
    int i;

    table->debug = 0;

    if (table->groups != NULL) {
        for (i = 0; i < table->group_count; i++) {
            bsal_hash_table_group_destroy(table->groups + i,
                            table->memory);
        }

        bsal_memory_pool_free(table->memory, table->groups);
        table->groups = NULL;
    }

}

void bsal_hash_table_delete(struct bsal_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;
    int code;
    uint64_t last_stride;

    if (table->groups == NULL) {
        return;
    }

    if (!table->deletion_is_enabled) {
        return;
    }

    code = bsal_hash_table_find_bucket(table, key, &group, &bucket_in_group,
                    BSAL_HASH_TABLE_OPERATION_DELETE, &last_stride);

    if (code == BSAL_HASH_TABLE_KEY_FOUND) {

#ifdef BSAL_HASH_TABLE_DEBUG
        printf("bsal_hash_table_delete code BSAL_HASH_TABLE_KEY_FOUND %i %i\n",
                        group, bucket_in_group);
#endif

        bsal_hash_table_group_delete(table->groups + group, bucket_in_group);
        table->elements--;
    }
}

int bsal_hash_table_get_group(struct bsal_hash_table *table, uint64_t bucket)
{
    return bucket / table->buckets_per_group;
}

int bsal_hash_table_get_group_bucket(struct bsal_hash_table *table, uint64_t bucket)
{
    return bucket % table->buckets_per_group;
}

/*
 * The hash function can be changed here.
 */
uint64_t bsal_hash_table_hash(void *key, int key_size, unsigned int seed)
{
    if (key_size < 0) {
        printf("DEBUG ERROR key_size %d\n", key_size);
    }

    return bsal_murmur_hash_2_64_a(key, key_size, seed);
}

uint64_t bsal_hash_table_hash1(struct bsal_hash_table *table, void *key)
{
    return bsal_hash_table_hash(key, table->key_size, 0x5cd902cb);
}

uint64_t bsal_hash_table_hash2(struct bsal_hash_table *table, void *key)
{
    uint64_t hash2;

    hash2 = bsal_hash_table_hash(key, table->key_size, 0x80435418);

    /* the number of buckets and hash2 must be co-prime
     * the number of buckets is a power of 2
     * must be between 1 and M - 1
     */
    if (hash2 == 0) {
        hash2 = 1;
    }
    if (hash2 % 2 == 0) {
        hash2--;
    }

    return hash2;
}

uint64_t bsal_hash_table_double_hash(struct bsal_hash_table *table, uint64_t hash1,
                uint64_t hash2, uint64_t stride)
{
    return (hash1 + stride * hash2) % table->buckets;
}

uint64_t bsal_hash_table_size(struct bsal_hash_table *table)
{
    return table->elements;
}

uint64_t bsal_hash_table_buckets(struct bsal_hash_table *table)
{
    return table->buckets;
}

void bsal_hash_table_toggle_debug(struct bsal_hash_table *table)
{
    if (table->debug) {
        table->debug = 0;
        return;
    }

    table->debug = 1;
}

int bsal_hash_table_state(struct bsal_hash_table *self, uint64_t bucket)
{
    int group;
    int bucket_in_group;
    struct bsal_hash_table_group *table_group;

    if (self->groups == NULL) {
        return BSAL_HASH_TABLE_BUCKET_EMPTY;
    }

    if (bucket >= bsal_hash_table_buckets(self)) {
        return BSAL_HASH_TABLE_BUCKET_EMPTY;
    }

    group = bsal_hash_table_get_group(self, bucket);
    bucket_in_group = bsal_hash_table_get_group_bucket(self, bucket);

    table_group = self->groups + group;

    return bsal_hash_table_group_state(table_group, bucket_in_group);
}

void *bsal_hash_table_key(struct bsal_hash_table *self, uint64_t bucket)
{
    int group;
    int bucket_in_group;
    struct bsal_hash_table_group *table_group;

    if (self->groups == NULL) {
        return NULL;
    }

    if (bucket >= bsal_hash_table_buckets(self)) {
        return NULL;
    }

    group = bsal_hash_table_get_group(self, bucket);
    bucket_in_group = bsal_hash_table_get_group_bucket(self, bucket);

    table_group = self->groups + group;

    return bsal_hash_table_group_key(table_group, bucket_in_group,
                    self->key_size, self->value_size);
}

void *bsal_hash_table_value(struct bsal_hash_table *self, uint64_t bucket)
{
    int group;
    int bucket_in_group;
    struct bsal_hash_table_group *table_group;

    if (self->groups == NULL) {
        return NULL;
    }

    if (bucket >= bsal_hash_table_buckets(self)) {
        return NULL;
    }

    group = bsal_hash_table_get_group(self, bucket);
    bucket_in_group = bsal_hash_table_get_group_bucket(self, bucket);

    table_group = self->groups + group;

    return bsal_hash_table_group_value(table_group, bucket_in_group,
                    self->key_size, self->value_size);
}

int bsal_hash_table_value_size(struct bsal_hash_table *self)
{
    return self->value_size;
}

int bsal_hash_table_key_size(struct bsal_hash_table *self)
{
    return self->key_size;
}

int bsal_hash_table_pack_size(struct bsal_hash_table *self)
{
    return bsal_hash_table_pack_unpack(self, NULL, BSAL_PACKER_OPERATION_DRY_RUN);
}

int bsal_hash_table_pack(struct bsal_hash_table *self, void *buffer)
{
    return bsal_hash_table_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_PACK);
}

int bsal_hash_table_unpack(struct bsal_hash_table *self, void *buffer)
{
#ifdef BSAL_HASH_TABLE_DEBUG
    printf("hash unpack\n");
#endif

    return bsal_hash_table_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_UNPACK);
}

int bsal_hash_table_pack_unpack(struct bsal_hash_table *self, void *buffer, int operation)
{
    /* implement packing for the hash table
     */
    struct bsal_packer packer;
    int offset;
    int i;
    /*uint64_t elements;*/

#ifdef BSAL_HASH_TABLE_DEBUG
    printf("hash pack/unpack %p\n", buffer);
#endif

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_process(&packer, &self->elements, sizeof(self->elements));
    bsal_packer_process(&packer, &self->buckets, sizeof(self->buckets));

    bsal_packer_process(&packer, &self->group_count, sizeof(self->group_count));
    bsal_packer_process(&packer, &self->buckets_per_group, sizeof(self->buckets_per_group));
    bsal_packer_process(&packer, &self->key_size, sizeof(self->key_size));
    bsal_packer_process(&packer, &self->value_size, sizeof(self->value_size));

    bsal_packer_process(&packer, &self->debug, sizeof(self->debug));
    bsal_packer_process(&packer, &self->deletion_is_enabled, sizeof(self->deletion_is_enabled));

    offset = bsal_packer_get_byte_count(&packer);

#ifdef BSAL_HASH_TABLE_DEBUG
    printf("before destroy\n");
#endif

    bsal_packer_destroy(&packer);

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

#ifdef BSAL_HASH_TABLE_DEBUG
        printf("hash init, buckets key_size value_size %d %d %d\n",
                         (int)self->buckets, self->key_size, self->value_size);
#endif
        /*
        elements = self->elements;
        self->elements = elements;
        */
        self->groups = NULL;
        bsal_hash_table_set_memory_pool(self, NULL);

        bsal_hash_table_start_groups(self);
    } else {

        /* The code does not support an empty map with
         * no groups.
         */
        if (self->groups == NULL) {
            bsal_hash_table_start_groups(self);
        }
    }

    for (i = 0; i < self->group_count; i++) {

        offset += bsal_hash_table_group_pack_unpack(self->groups + i,
                        (char *)buffer + offset, operation,
                        self->buckets_per_group, self->key_size,
                        self->value_size, self->memory,
                        self->deletion_is_enabled);
    }

    return offset;
}

void bsal_hash_table_start_groups(struct bsal_hash_table *table)
{
    int i;

    if (table->groups != NULL) {
        return;
    }

#ifdef BSAL_HASH_TABLE_DEBUG_INIT
    printf("DEBUG bsal_hash_table_start_groups %p\n", (void *)table);
#endif

    table->groups = (struct bsal_hash_table_group *)
            bsal_memory_pool_allocate(table->memory,
                            table->group_count * sizeof(struct bsal_hash_table_group));

#ifdef BSAL_HASH_TABLE_DEBUG_INIT
    printf("DEBUG bsal_hash_table_init group_count %d\n",
                    table->group_count);
#endif

    for (i = 0; i < table->group_count; i++) {
        bsal_hash_table_group_init(table->groups + i, table->buckets_per_group,
                        table->key_size, table->value_size, table->memory, table->deletion_is_enabled);
    }
}

void bsal_hash_table_set_memory_pool(struct bsal_hash_table *table, struct bsal_memory_pool *memory)
{
    table->memory = memory;
}

void bsal_hash_table_disable_deletion_support(struct bsal_hash_table *table)
{
    table->deletion_is_enabled = 0;
}

void bsal_hash_table_enable_deletion_support(struct bsal_hash_table *table)
{

    table->deletion_is_enabled = 1;
}

int bsal_hash_table_deletion_support_is_enabled(struct bsal_hash_table *table)
{
    return table->deletion_is_enabled;
}

void *bsal_hash_table_add(struct bsal_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;
    void *bucket_key;
    int code;
    uint64_t last_stride;

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
    int query_value;
    int key_value;

    if (table->debug) {
        printf("DEBUG bsal_hash_table_add\n");
    }
#endif

    if (table->groups == NULL) {
        bsal_hash_table_start_groups(table);
    }

    code = bsal_hash_table_find_bucket(table, key, &group, &bucket_in_group,
                    BSAL_HASH_TABLE_OPERATION_ADD, &last_stride);

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG_STRIDE

    printf("STRIDE load %" PRIu64 "/%" PRIu64 " stride %" PRIu64 "\n",
                    bsal_hash_table_size(table),
                    bsal_hash_table_buckets(table),
                    last_stride);
#endif

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
    if (table->debug) {
        printf("DEBUG bsal_hash_table_add group %d bucket_in_group %d code %d\n",
                        group, bucket_in_group, code);
    }
#endif

    if (code == BSAL_HASH_TABLE_KEY_NOT_FOUND) {

#ifdef BSAL_HASH_TABLE_DEBUG
        printf("bsal_hash_table_add code BSAL_HASH_TABLE_KEY_NOT_FOUND"
                        "(group %i bucket %i)\n", group, bucket_in_group);
#endif

        /* install the key */
        bucket_key = bsal_hash_table_group_key(table->groups + group,
                        bucket_in_group, table->key_size, table->value_size);

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (table->debug) {
            printf("DEBUG get key group %d bucket_in_group %d key_size %d value_size %d\n",
                            group, bucket_in_group, table->key_size, table->value_size);

            printf("DEBUG bsal_memory_copy %p %p %i\n", bucket_key, key, table->key_size);
        }
#endif

        bsal_memory_copy(bucket_key, key, table->key_size);

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (table->debug) {

            query_value = *(int *)key;
            key_value = *(int *)bucket_key;

            printf("DEBUG after copy query_value %d key_value %d bucket %p\n",
                            query_value, key_value, bucket_key);
        }
#endif

        table->elements++;

        return bsal_hash_table_group_add(table->groups + group, bucket_in_group,
                   table->key_size, table->value_size);

    } else if (code == BSAL_HASH_TABLE_KEY_FOUND) {

#ifdef BSAL_HASH_TABLE_DEBUG
        printf("bsal_hash_table_add code BSAL_HASH_TABLE_KEY_FOUND"
                        "(group %i bucket %i)\n", group, bucket_in_group);
#endif

        return bsal_hash_table_group_get(table->groups + group, bucket_in_group,
                   table->key_size, table->value_size);

    } else if (code == BSAL_HASH_TABLE_FULL) {

#ifdef BSAL_HASH_TABLE_DEBUG
        printf("bsal_hash_table_add code BSAL_HASH_TABLE_FULL\n");
#endif

        return NULL;
    }

    /* this statement is unreachable.
     */
    return NULL;
}

void *bsal_hash_table_get(struct bsal_hash_table *table, void *key)
{
    int group;
    int bucket_in_group;
    int code;
    uint64_t last_stride;

    if (table->groups == NULL) {
        return NULL;
    }

    code = bsal_hash_table_find_bucket(table, key, &group, &bucket_in_group,
                    BSAL_HASH_TABLE_OPERATION_GET, &last_stride);

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
    if (table->debug) {
        printf("DEBUG bsal_hash_table_get key %p group %d bucket_in_group %d code %d\n",
                        key, group, bucket_in_group, code);
    }
#endif

    /* bsal_hash_table_group_get would return NULL too,
     * but using this return code is cleaner */
    if (code == BSAL_HASH_TABLE_KEY_NOT_FOUND) {
        return NULL;
    }

#ifdef BSAL_HASH_TABLE_DEBUG
    printf("get %i %i code %i\n", group, bucket_in_group,
                    code);
#endif

    return bsal_hash_table_group_get(table->groups + group, bucket_in_group,
                    table->key_size, table->value_size);
}


/* this is the most important function for the hash table.
 * it finds a bucket with a key
 *
 * \param operation is one of these: BSAL_HASH_TABLE_OPERATION_ADD,
 * BSAL_HASH_TABLE_OPERATION_GET, BSAL_HASH_TABLE_OPERATION_DELETE
 *
 * \return value is BSAL_HASH_TABLE_KEY_FOUND or BSAL_HASH_TABLE_KEY_NOT_FOUND or
 * BSAL_HASH_TABLE_FULL
 */
int bsal_hash_table_find_bucket(struct bsal_hash_table *table, void *key,
                int *group, int *bucket_in_group, int operation,
                uint64_t *last_stride)
{
    uint64_t bucket;
    uint64_t stride;
    int state;
    struct bsal_hash_table_group *hash_group;
    void *bucket_key;
    uint64_t hash1;
    uint64_t hash2;
    int local_group;
    int local_bucket_in_group;
    int operation_is_delete_or_get;
    int operation_is_add;

    operation_is_delete_or_get = 0;
    operation_is_add = 0;

    if (operation & BSAL_HASH_TABLE_OPERATION_DELETE
                      ||
                  operation & BSAL_HASH_TABLE_OPERATION_GET) {

        operation_is_delete_or_get = 1;

    } else if (operation & BSAL_HASH_TABLE_OPERATION_ADD) {

        operation_is_add = 1;
    }

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
    int query_value;
    int key_value;

    if (table->debug) {
        printf("DEBUG bsal_hash_table_find_bucket\n");
    }
#endif

    stride = 0;

    hash1 = bsal_hash_table_hash1(table, key);
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
#ifdef BSAL_HASH_TABLE_USE_ONE_GROUP
    hash_group = table->groups + local_group;
#endif

    while (stride < table->buckets) {

        /* compute hash2 only on the second stride
         * It is not needed for stride # 0.
         * And strides #1, #2, #3 will used the same value.
         */
        if (stride == 1) {
            hash2 = bsal_hash_table_hash2(table, key);
        }

        bucket = bsal_hash_table_double_hash(table, hash1, hash2, stride);
        local_bucket_in_group = bsal_hash_table_get_group_bucket(table, bucket);

        /* Get the hash group if there is more than
         * one hash group.
         */
#ifndef BSAL_HASH_TABLE_USE_ONE_GROUP
        local_group = bsal_hash_table_get_group(table, bucket);
        hash_group = table->groups + local_group;
#endif

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (table->groups == NULL) {
            printf("DEBUG %p Error groups is %p\n", (void *)table,
                            (void *)table->groups);
        }
#endif

        state = bsal_hash_table_group_state(hash_group, local_bucket_in_group);

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (table->debug) {
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
        if (state & BSAL_HASH_TABLE_BUCKET_OCCUPIED) {

            /* the bucket is occupied, compare it with the key */
            bucket_key = bsal_hash_table_group_key(hash_group, local_bucket_in_group,
                        table->key_size, table->value_size);


            if (memcmp(bucket_key, key, table->key_size) ==
                BSAL_HASH_TABLE_MATCH) {

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
                if (table->debug) {
                    printf("DEBUG state= OCCUPIED, match !\n");
                }
#endif
                *group = local_group;
                *bucket_in_group = local_bucket_in_group;
                *last_stride = stride;

                return BSAL_HASH_TABLE_KEY_FOUND;
            }
        }

        /*
         * Case 2.
         *
         * We found an empty bucket to fulfil the procurement.
         */
        if (state & BSAL_HASH_TABLE_BUCKET_EMPTY) {

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
            if (table->debug) {
                printf("DEBUG state= EMPTY\n");
            }
#endif

            *group = local_group;
            *bucket_in_group = local_bucket_in_group;
            *last_stride = stride;

            return BSAL_HASH_TABLE_KEY_NOT_FOUND;
        }



        /*
         * Case 3.
         *
         * The bucket is deleted, and the operation is DELETE or GET.
         * In that case, this bucket is useless.
         *
         * Nothing to see here, it is deleted !
         * we only pick it up for BSAL_HASH_TABLE_OPERATION_ADD
         * \see http://webdocs.cs.ualberta.ca/~holte/T26/open-addr.html
         */
        if (state & BSAL_HASH_TABLE_BUCKET_DELETED
              && operation_is_delete_or_get) {

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
            if (table->debug) {
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
        if (state & BSAL_HASH_TABLE_BUCKET_DELETED
               && operation_is_add) {

#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
            if (table->debug) {
                printf("DEBUG state= DELETED, op= ADD\n");
            }
#endif

            *group = local_group;
            *bucket_in_group = local_bucket_in_group;
            *last_stride = stride;

            return BSAL_HASH_TABLE_KEY_NOT_FOUND;
        }

        /*
         * No match was found
         */
#ifdef BSAL_HASH_TABLE_DEBUG_DOUBLE_HASHING_DEBUG
        if (table->debug) {
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
        return BSAL_HASH_TABLE_FULL;
    }

    if (operation_is_delete_or_get) {
        return BSAL_HASH_TABLE_KEY_NOT_FOUND;
    }

    /* This statement can not be reached, assuming the operation
     * is ADD, GET, or DELETE
     * UPDATE is implemented on top of this hash table, not directly inside
     * of it.
     */
    return BSAL_HASH_TABLE_FULL;
}

void bsal_hash_table_clear(struct bsal_hash_table *self)
{
    int key_size;
    int value_size;
    struct bsal_memory_pool *pool;
    uint64_t buckets;

    key_size = self->key_size;
    value_size = self->value_size;
    pool = self->memory;
    buckets = self->buckets;

    bsal_hash_table_destroy(self);

    bsal_hash_table_init(self, key_size, value_size, buckets);

    if (pool != NULL) {
        bsal_hash_table_set_memory_pool(self, pool);
    }
}

struct bsal_memory_pool *bsal_hash_table_memory_pool(struct bsal_hash_table *self)
{
    return self->memory;
}
