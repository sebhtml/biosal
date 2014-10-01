
#include "map.h"

#include <core/system/memory.h>

#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <inttypes.h>

#define BIOSAL_MAP_ENABLE_ESTIMATION

void biosal_map_init(struct biosal_map *self, int key_size, int value_size)
{
    uint64_t buckets = 2;

    biosal_map_init_with_capacity(self, key_size, value_size, buckets);
}

void biosal_map_init_with_capacity(struct biosal_map *self, int key_size, int value_size, uint64_t buckets)
{
#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    self->original_key_size = key_size;
    self->original_value_size = value_size;

    key_size = biosal_memory_align(key_size);
    value_size = biosal_memory_align(value_size);

    self->key_padding = key_size - self->original_key_size;
    self->key_buffer = biosal_memory_allocate(key_size);
#endif

    biosal_dynamic_hash_table_init(&self->table, buckets, key_size, value_size);
}

void biosal_map_destroy(struct biosal_map *self)
{
    biosal_dynamic_hash_table_destroy(&self->table);

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    biosal_memory_free(self->key_buffer);
    self->key_buffer = NULL;
#endif
}

void *biosal_map_add(struct biosal_map *self, void *key)
{
#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    key = biosal_map_pad_key(self, key);
#endif

    return biosal_dynamic_hash_table_add(&self->table, key);
}

void *biosal_map_get(struct biosal_map *self, void *key)
{
#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    key = biosal_map_pad_key(self, key);
#endif

    return biosal_dynamic_hash_table_get(&self->table, key);
}

void biosal_map_delete(struct biosal_map *self, void *key)
{
#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    key = biosal_map_pad_key(self, key);
#endif

    biosal_dynamic_hash_table_delete(&self->table, key);
}

uint64_t biosal_map_size(struct biosal_map *self)
{
    return biosal_dynamic_hash_table_size(&self->table);
}

struct biosal_dynamic_hash_table *biosal_map_table(struct biosal_map *self)
{
    return &self->table;
}

int biosal_map_pack_size(struct biosal_map *self)
{
    return biosal_map_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK_SIZE, NULL);
}

int biosal_map_pack(struct biosal_map *self, void *buffer)
{
    return biosal_map_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK, buffer);
}

int biosal_map_unpack(struct biosal_map *self, void *buffer)
{
    int value;

#ifdef BIOSAL_MAP_DEBUG
    printf("DEBUG map_unpack\n");
#endif

    value = biosal_map_pack_unpack(self, BIOSAL_PACKER_OPERATION_UNPACK, buffer);

#ifdef BIOSAL_MAP_DEBUG
    printf("DEBUG map_unpack after\n");
#endif

    return value;
}

int biosal_map_update_value(struct biosal_map *self, void *key, void *value)
{
    void *bucket;
    int value_size;

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    key = biosal_map_pad_key(self, key);
#endif

    bucket = biosal_map_get(self, key);

    if (bucket == NULL) {
        return 0;
    }

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    value_size = self->original_value_size;
#else
    value_size = biosal_map_get_value_size(self);
#endif

    biosal_memory_copy(bucket, value, value_size);

    return 1;
}

int biosal_map_add_value(struct biosal_map *self, void *key, void *value)
{
    void *bucket;
    int value_size;

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    key = biosal_map_pad_key(self, key);
#endif

    bucket = biosal_map_get(self, key);

    /* it's already there...
     */
    if (bucket != NULL) {
        return 0;
    }

    bucket = biosal_map_add(self, key);

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    value_size = self->original_value_size;
#else
    value_size = biosal_map_get_value_size(self);
#endif

    biosal_memory_copy(bucket, value, value_size);

    return 1;
}

int biosal_map_get_key_size(struct biosal_map *self)
{
#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    return self->original_key_size;
#else
    return biosal_dynamic_hash_table_get_key_size(&self->table);
#endif
}

int biosal_map_get_value_size(struct biosal_map *self)
{
#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    return self->original_value_size;
#else
    return biosal_dynamic_hash_table_get_value_size(&self->table);
#endif
}

int biosal_map_empty(struct biosal_map *self)
{
    return biosal_map_size(self) == 0;
}

int biosal_map_get_value(struct biosal_map *self, void *key, void *value)
{
    void *bucket;
    int size;

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    key = biosal_map_pad_key(self, key);
#endif

    bucket = biosal_map_get(self, key);

    if (bucket == NULL) {
        return 0;
    }

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    size = self->original_value_size;
#else
    size = biosal_map_get_value_size(self);
#endif

    if (value != NULL) {
        biosal_memory_copy(value, bucket, size);
    }

    return 1;
}

int biosal_map_pack_unpack(struct biosal_map *self, int operation, void *buffer)
{
    struct biosal_packer packer;
    int offset;

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    int key_size;
#endif

    BIOSAL_DEBUGGER_ASSERT(self != NULL);

    biosal_packer_init(&packer, operation, buffer);

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
    biosal_packer_process(&packer, &self->original_key_size, sizeof(self->original_key_size));
    biosal_packer_process(&packer, &self->original_value_size, sizeof(self->original_value_size));
#endif

    offset = biosal_packer_get_byte_count(&packer);
    biosal_packer_destroy(&packer);

    if (operation == BIOSAL_PACKER_OPERATION_PACK_SIZE) {
        offset += biosal_dynamic_hash_table_pack_size(&self->table);

    } else if (operation == BIOSAL_PACKER_OPERATION_PACK) {
        offset += biosal_dynamic_hash_table_pack(&self->table, (char *)buffer + offset);

    } else if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {
        offset += biosal_dynamic_hash_table_unpack(&self->table, (char *)buffer + offset);

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
        key_size = biosal_dynamic_hash_table_get_key_size(&self->table);

        self->key_padding = key_size - self->original_key_size;

        self->key_buffer = biosal_memory_allocate(key_size);
#endif
    }

    return offset;
}

#ifdef BIOSAL_MAP_ALIGNMENT_ENABLED
void *biosal_map_pad_key(struct biosal_map *self, void *key)
{
    if (self->key_padding == 0) {
        return key;
    }

    /* if alignment is used, point the key to an alignment
     * version of it
     */

    biosal_memory_copy(self->key_buffer, key, self->original_key_size);
    memset((char *)self->key_buffer + self->original_key_size, 0,
                self->key_padding);

    return self->key_buffer;
}
#endif

void biosal_map_set_memory_pool(struct biosal_map *map, struct biosal_memory_pool *memory)
{
    biosal_dynamic_hash_table_set_memory_pool(&map->table, memory);
}

void biosal_map_disable_deletion_support(struct biosal_map *map)
{
    biosal_dynamic_hash_table_disable_deletion_support(&map->table);
}

void biosal_map_enable_deletion_support(struct biosal_map *map)
{
    biosal_dynamic_hash_table_enable_deletion_support(&map->table);

}

void biosal_map_set_current_size_estimate(struct biosal_map *map, double value)
{
#ifdef BIOSAL_MAP_ENABLE_ESTIMATION
    biosal_dynamic_hash_table_set_current_size_estimate(&map->table, value);
#endif
}

void biosal_map_set_threshold(struct biosal_map *map, double threshold)
{
    biosal_dynamic_hash_table_set_threshold(&map->table, threshold);
}

int biosal_map_is_currently_resizing(struct biosal_map *map)
{
    return biosal_dynamic_hash_table_is_currently_resizing(&map->table);
}

void biosal_map_clear(struct biosal_map *self)
{
    int key_size;
    int value_size;

    key_size = biosal_map_get_key_size(self);
    value_size = biosal_map_get_value_size(self);

    biosal_map_destroy(self);

    biosal_map_init(self, key_size, value_size);
    /*biosal_dynamic_hash_table_clear(&self->table);*/
}

void biosal_map_examine(struct biosal_map *self)
{
    int key_size;
    int value_size;
    uint64_t size;

    key_size = biosal_map_get_key_size(self);
    value_size = biosal_map_get_value_size(self);
    size = biosal_map_size(self);

    printf("biosal_map_examine key_size %d value_size %d size %" PRIu64 "\n",
                    key_size, value_size, size);
}
