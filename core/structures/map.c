
#include "map.h"

#include <core/system/memory.h>

#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <inttypes.h>

#define BSAL_MAP_ENABLE_ESTIMATION

void bsal_map_init(struct bsal_map *self, int key_size, int value_size)
{
    uint64_t buckets = 2;

    bsal_map_init_with_capacity(self, key_size, value_size, buckets);
}

void bsal_map_init_with_capacity(struct bsal_map *self, int key_size, int value_size, uint64_t buckets)
{
#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    self->original_key_size = key_size;
    self->original_value_size = value_size;

    key_size = bsal_memory_align(key_size);
    value_size = bsal_memory_align(value_size);

    self->key_padding = key_size - self->original_key_size;
    self->key_buffer = bsal_memory_allocate(key_size);
#endif

    bsal_dynamic_hash_table_init(&self->table, buckets, key_size, value_size);
}

void bsal_map_destroy(struct bsal_map *self)
{
    bsal_dynamic_hash_table_destroy(&self->table);

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    bsal_memory_free(self->key_buffer);
    self->key_buffer = NULL;
#endif
}

void *bsal_map_add(struct bsal_map *self, void *key)
{
#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    key = bsal_map_pad_key(self, key);
#endif

    return bsal_dynamic_hash_table_add(&self->table, key);
}

void *bsal_map_get(struct bsal_map *self, void *key)
{
#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    key = bsal_map_pad_key(self, key);
#endif

    return bsal_dynamic_hash_table_get(&self->table, key);
}

void bsal_map_delete(struct bsal_map *self, void *key)
{
#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    key = bsal_map_pad_key(self, key);
#endif

    bsal_dynamic_hash_table_delete(&self->table, key);
}

uint64_t bsal_map_size(struct bsal_map *self)
{
    return bsal_dynamic_hash_table_size(&self->table);
}

struct bsal_dynamic_hash_table *bsal_map_table(struct bsal_map *self)
{
    return &self->table;
}

int bsal_map_pack_size(struct bsal_map *self)
{
    return bsal_map_pack_unpack(self, BSAL_PACKER_OPERATION_PACK_SIZE, NULL);
}

int bsal_map_pack(struct bsal_map *self, void *buffer)
{
    return bsal_map_pack_unpack(self, BSAL_PACKER_OPERATION_PACK, buffer);
}

int bsal_map_unpack(struct bsal_map *self, void *buffer)
{
    int value;

#ifdef BSAL_MAP_DEBUG
    printf("DEBUG map_unpack\n");
#endif

    value = bsal_map_pack_unpack(self, BSAL_PACKER_OPERATION_UNPACK, buffer);

#ifdef BSAL_MAP_DEBUG
    printf("DEBUG map_unpack after\n");
#endif

    return value;
}

int bsal_map_update_value(struct bsal_map *self, void *key, void *value)
{
    void *bucket;
    int value_size;

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    key = bsal_map_pad_key(self, key);
#endif

    bucket = bsal_map_get(self, key);

    if (bucket == NULL) {
        return 0;
    }

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    value_size = self->original_value_size;
#else
    value_size = bsal_map_get_value_size(self);
#endif

    bsal_memory_copy(bucket, value, value_size);

    return 1;
}

int bsal_map_add_value(struct bsal_map *self, void *key, void *value)
{
    void *bucket;
    int value_size;

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    key = bsal_map_pad_key(self, key);
#endif

    bucket = bsal_map_get(self, key);

    /* it's already there...
     */
    if (bucket != NULL) {
        return 0;
    }

    bucket = bsal_map_add(self, key);

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    value_size = self->original_value_size;
#else
    value_size = bsal_map_get_value_size(self);
#endif

    bsal_memory_copy(bucket, value, value_size);

    return 1;
}

int bsal_map_get_key_size(struct bsal_map *self)
{
#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    return self->original_key_size;
#else
    return bsal_dynamic_hash_table_get_key_size(&self->table);
#endif
}

int bsal_map_get_value_size(struct bsal_map *self)
{
#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    return self->original_value_size;
#else
    return bsal_dynamic_hash_table_get_value_size(&self->table);
#endif
}

int bsal_map_empty(struct bsal_map *self)
{
    return bsal_map_size(self) == 0;
}

int bsal_map_get_value(struct bsal_map *self, void *key, void *value)
{
    void *bucket;
    int size;

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    key = bsal_map_pad_key(self, key);
#endif

    bucket = bsal_map_get(self, key);

    if (bucket == NULL) {
        return 0;
    }

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    size = self->original_value_size;
#else
    size = bsal_map_get_value_size(self);
#endif

    if (value != NULL) {
        bsal_memory_copy(value, bucket, size);
    }

    return 1;
}

int bsal_map_pack_unpack(struct bsal_map *self, int operation, void *buffer)
{
    struct bsal_packer packer;
    int offset;

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    int key_size;
#endif

    BSAL_DEBUGGER_ASSERT(self != NULL);

    bsal_packer_init(&packer, operation, buffer);

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
    bsal_packer_process(&packer, &self->original_key_size, sizeof(self->original_key_size));
    bsal_packer_process(&packer, &self->original_value_size, sizeof(self->original_value_size));
#endif

    offset = bsal_packer_get_byte_count(&packer);
    bsal_packer_destroy(&packer);

    if (operation == BSAL_PACKER_OPERATION_PACK_SIZE) {
        offset += bsal_dynamic_hash_table_pack_size(&self->table);

    } else if (operation == BSAL_PACKER_OPERATION_PACK) {
        offset += bsal_dynamic_hash_table_pack(&self->table, (char *)buffer + offset);

    } else if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        offset += bsal_dynamic_hash_table_unpack(&self->table, (char *)buffer + offset);

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
        key_size = bsal_dynamic_hash_table_get_key_size(&self->table);

        self->key_padding = key_size - self->original_key_size;

        self->key_buffer = bsal_memory_allocate(key_size);
#endif
    }

    return offset;
}

#ifdef BSAL_MAP_ALIGNMENT_ENABLED
void *bsal_map_pad_key(struct bsal_map *self, void *key)
{
    if (self->key_padding == 0) {
        return key;
    }

    /* if alignment is used, point the key to an alignment
     * version of it
     */

    bsal_memory_copy(self->key_buffer, key, self->original_key_size);
    memset((char *)self->key_buffer + self->original_key_size, 0,
                self->key_padding);

    return self->key_buffer;
}
#endif

void bsal_map_set_memory_pool(struct bsal_map *map, struct bsal_memory_pool *memory)
{
    bsal_dynamic_hash_table_set_memory_pool(&map->table, memory);
}

void bsal_map_disable_deletion_support(struct bsal_map *map)
{
    bsal_dynamic_hash_table_disable_deletion_support(&map->table);
}

void bsal_map_enable_deletion_support(struct bsal_map *map)
{
    bsal_dynamic_hash_table_enable_deletion_support(&map->table);

}

void bsal_map_set_current_size_estimate(struct bsal_map *map, double value)
{
#ifdef BSAL_MAP_ENABLE_ESTIMATION
    bsal_dynamic_hash_table_set_current_size_estimate(&map->table, value);
#endif
}

void bsal_map_set_threshold(struct bsal_map *map, double threshold)
{
    bsal_dynamic_hash_table_set_threshold(&map->table, threshold);
}

int bsal_map_is_currently_resizing(struct bsal_map *map)
{
    return bsal_dynamic_hash_table_is_currently_resizing(&map->table);
}

void bsal_map_clear(struct bsal_map *self)
{
    int key_size;
    int value_size;

    key_size = bsal_map_get_key_size(self);
    value_size = bsal_map_get_value_size(self);

    bsal_map_destroy(self);

    bsal_map_init(self, key_size, value_size);
    /*bsal_dynamic_hash_table_clear(&self->table);*/
}

void bsal_map_examine(struct bsal_map *self)
{
    int key_size;
    int value_size;
    uint64_t size;

    key_size = bsal_map_get_key_size(self);
    value_size = bsal_map_get_value_size(self);
    size = bsal_map_size(self);

    printf("bsal_map_examine key_size %d value_size %d size %" PRIu64 "\n",
                    key_size, value_size, size);
}
