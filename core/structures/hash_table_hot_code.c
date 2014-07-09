
#include "hash_table.h"

#include <string.h>

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

            printf("DEBUG memcpy %p %p %i\n", bucket_key, key, table->key_size);
        }
#endif

        memcpy(bucket_key, key, table->key_size);

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


