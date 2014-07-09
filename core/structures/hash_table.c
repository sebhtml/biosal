
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

    table->groups = NULL;

    bsal_hash_table_set_memory_pool(table, NULL);
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
    uint64_t elements;

#ifdef BSAL_HASH_TABLE_DEBUG
    printf("hash pack/unpack %p\n", buffer);
#endif

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, &self->elements, sizeof(self->elements));
    bsal_packer_work(&packer, &self->buckets, sizeof(self->buckets));

    bsal_packer_work(&packer, &self->group_count, sizeof(self->group_count));
    bsal_packer_work(&packer, &self->buckets_per_group, sizeof(self->buckets_per_group));
    bsal_packer_work(&packer, &self->key_size, sizeof(self->key_size));
    bsal_packer_work(&packer, &self->value_size, sizeof(self->value_size));

    bsal_packer_work(&packer, &self->debug, sizeof(self->debug));

    offset = bsal_packer_worked_bytes(&packer);

#ifdef BSAL_HASH_TABLE_DEBUG
    printf("before destroy\n");
#endif

    bsal_packer_destroy(&packer);

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

#ifdef BSAL_HASH_TABLE_DEBUG
        printf("hash init, buckets key_size value_size %d %d %d\n",
                         (int)self->buckets, self->key_size, self->value_size);
#endif
        elements = self->elements;
        bsal_hash_table_init(self, self->buckets, self->key_size, self->value_size);
        self->elements = elements;

        if (self->groups == NULL) {
            bsal_hash_table_start_groups(self);
        }
    }

    for (i = 0; i < self->group_count; i++) {

        offset += bsal_hash_table_group_pack_unpack(self->groups + i,
                        (char *)buffer + offset, operation,
                        self->buckets_per_group, self->key_size,
                        self->value_size, self->memory);
    }

    return offset;
}

void bsal_hash_table_start_groups(struct bsal_hash_table *table)
{
    int i;

    table->groups = (struct bsal_hash_table_group *)
            bsal_memory_pool_allocate(table->memory,
                            table->group_count * sizeof(struct bsal_hash_table_group));

#ifdef BSAL_HASH_TABLE_DEBUG_INIT
    printf("DEBUG bsal_hash_table_init group_count %d\n",
                    table->group_count);
#endif

    for (i = 0; i < table->group_count; i++) {
        bsal_hash_table_group_init(table->groups + i, table->buckets_per_group,
                        table->key_size, table->value_size, table->memory);
    }
}

void bsal_hash_table_set_memory_pool(struct bsal_hash_table *table, struct bsal_memory_pool *memory)
{
    table->memory = memory;
}


