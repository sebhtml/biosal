
#include "map.h"

#include <core/system/memory.h>

#include <core/system/packer.h>
#include <core/system/debugger.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <inttypes.h>

#define CORE_MAP_ENABLE_ESTIMATION

void core_map_init(struct core_map *self, int key_size, int value_size)
{
    uint64_t buckets = 2;

    core_map_init_with_capacity(self, key_size, value_size, buckets);
}

void core_map_init_with_capacity(struct core_map *self, int key_size, int value_size, uint64_t buckets)
{
#ifdef CORE_MAP_ALIGNMENT_ENABLED
    self->original_key_size = key_size;
    self->original_value_size = value_size;

    key_size = core_memory_align(key_size);
    value_size = core_memory_align(value_size);

    self->key_padding = key_size - self->original_key_size;
    self->key_buffer = core_memory_allocate(key_size);
#endif

    core_dynamic_hash_table_init(&self->table, buckets, key_size, value_size);
}

void core_map_destroy(struct core_map *self)
{
    core_dynamic_hash_table_destroy(&self->table);

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    core_memory_free(self->key_buffer);
    self->key_buffer = NULL;
#endif
}

void *core_map_add(struct core_map *self, void *key)
{
#ifdef CORE_MAP_ALIGNMENT_ENABLED
    key = core_map_pad_key(self, key);
#endif

    return core_dynamic_hash_table_add(&self->table, key);
}

void *core_map_get(struct core_map *self, void *key)
{
#ifdef CORE_MAP_ALIGNMENT_ENABLED
    key = core_map_pad_key(self, key);
#endif

    return core_dynamic_hash_table_get(&self->table, key);
}

void core_map_delete(struct core_map *self, void *key)
{
#ifdef CORE_MAP_ALIGNMENT_ENABLED
    key = core_map_pad_key(self, key);
#endif

    core_dynamic_hash_table_delete(&self->table, key);
}

uint64_t core_map_size(struct core_map *self)
{
    return core_dynamic_hash_table_size(&self->table);
}

struct core_dynamic_hash_table *core_map_table(struct core_map *self)
{
    return &self->table;
}

int core_map_pack_size(struct core_map *self)
{
    return core_map_pack_unpack(self, CORE_PACKER_OPERATION_PACK_SIZE, NULL);
}

int core_map_pack(struct core_map *self, void *buffer)
{
    return core_map_pack_unpack(self, CORE_PACKER_OPERATION_PACK, buffer);
}

int core_map_unpack(struct core_map *self, void *buffer)
{
    int value;

#ifdef CORE_MAP_DEBUG
    printf("DEBUG map_unpack\n");
#endif

    value = core_map_pack_unpack(self, CORE_PACKER_OPERATION_UNPACK, buffer);

#ifdef CORE_MAP_DEBUG
    printf("DEBUG map_unpack after\n");
#endif

    return value;
}

int core_map_update_value(struct core_map *self, void *key, void *value)
{
    void *bucket;
    int value_size;

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    key = core_map_pad_key(self, key);
#endif

    bucket = core_map_get(self, key);

    if (bucket == NULL) {
        return 0;
    }

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    value_size = self->original_value_size;
#else
    value_size = core_map_get_value_size(self);
#endif

    core_memory_copy(bucket, value, value_size);

    return 1;
}

int core_map_add_value(struct core_map *self, void *key, void *value)
{
    void *bucket;
    int value_size;

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    key = core_map_pad_key(self, key);
#endif

    bucket = core_map_get(self, key);

    /* it's already there...
     */
    if (bucket != NULL) {
        return 0;
    }

    bucket = core_map_add(self, key);

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    value_size = self->original_value_size;
#else
    value_size = core_map_get_value_size(self);
#endif

    core_memory_copy(bucket, value, value_size);

    return 1;
}

int core_map_get_key_size(struct core_map *self)
{
#ifdef CORE_MAP_ALIGNMENT_ENABLED
    return self->original_key_size;
#else
    return core_dynamic_hash_table_get_key_size(&self->table);
#endif
}

int core_map_get_value_size(struct core_map *self)
{
#ifdef CORE_MAP_ALIGNMENT_ENABLED
    return self->original_value_size;
#else
    return core_dynamic_hash_table_get_value_size(&self->table);
#endif
}

int core_map_empty(struct core_map *self)
{
    return core_map_size(self) == 0;
}

int core_map_get_value(struct core_map *self, void *key, void *value)
{
    void *bucket;
    int size;

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    key = core_map_pad_key(self, key);
#endif

    bucket = core_map_get(self, key);

    if (bucket == NULL) {
        return 0;
    }

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    size = self->original_value_size;
#else
    size = core_map_get_value_size(self);
#endif

    if (value != NULL) {
        core_memory_copy(value, bucket, size);
    }

    return 1;
}

int core_map_pack_unpack(struct core_map *self, int operation, void *buffer)
{
    struct core_packer packer;
    int offset;

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    int key_size;
#endif

    CORE_DEBUGGER_ASSERT(self != NULL);

    core_packer_init(&packer, operation, buffer);

#ifdef CORE_MAP_ALIGNMENT_ENABLED
    core_packer_process(&packer, &self->original_key_size, sizeof(self->original_key_size));
    core_packer_process(&packer, &self->original_value_size, sizeof(self->original_value_size));
#endif

    offset = core_packer_get_byte_count(&packer);
    core_packer_destroy(&packer);

    if (operation == CORE_PACKER_OPERATION_PACK_SIZE) {
        offset += core_dynamic_hash_table_pack_size(&self->table);

    } else if (operation == CORE_PACKER_OPERATION_PACK) {
        offset += core_dynamic_hash_table_pack(&self->table, (char *)buffer + offset);

    } else if (operation == CORE_PACKER_OPERATION_UNPACK) {
        offset += core_dynamic_hash_table_unpack(&self->table, (char *)buffer + offset);

#ifdef CORE_MAP_ALIGNMENT_ENABLED
        key_size = core_dynamic_hash_table_get_key_size(&self->table);

        self->key_padding = key_size - self->original_key_size;

        self->key_buffer = core_memory_allocate(key_size);
#endif
    }

    return offset;
}

#ifdef CORE_MAP_ALIGNMENT_ENABLED
void *core_map_pad_key(struct core_map *self, void *key)
{
    if (self->key_padding == 0) {
        return key;
    }

    /* if alignment is used, point the key to an alignment
     * version of it
     */

    core_memory_copy(self->key_buffer, key, self->original_key_size);
    memset((char *)self->key_buffer + self->original_key_size, 0,
                self->key_padding);

    return self->key_buffer;
}
#endif

void core_map_set_memory_pool(struct core_map *map, struct core_memory_pool *memory)
{
    core_dynamic_hash_table_set_memory_pool(&map->table, memory);
}

void core_map_disable_deletion_support(struct core_map *map)
{
    core_dynamic_hash_table_disable_deletion_support(&map->table);
}

void core_map_enable_deletion_support(struct core_map *map)
{
    core_dynamic_hash_table_enable_deletion_support(&map->table);

}

void core_map_set_current_size_estimate(struct core_map *map, double value)
{
#ifdef CORE_MAP_ENABLE_ESTIMATION
    core_dynamic_hash_table_set_current_size_estimate(&map->table, value);
#endif
}

void core_map_set_threshold(struct core_map *map, double threshold)
{
    core_dynamic_hash_table_set_threshold(&map->table, threshold);
}

int core_map_is_currently_resizing(struct core_map *map)
{
    return core_dynamic_hash_table_is_currently_resizing(&map->table);
}

void core_map_clear(struct core_map *self)
{
    int key_size;
    int value_size;
    struct core_memory_pool *pool;

    /*
     * Save key_size, value_size, and memory pool.
     */
    key_size = core_map_get_key_size(self);
    value_size = core_map_get_value_size(self);
    pool = core_map_memory_pool(self);

    core_map_destroy(self);

    core_map_init(self, key_size, value_size);
    core_map_set_memory_pool(self, pool);

    /*
     * TODO: implement the clear operation directly inside
     * the lower layer (core_dynamic_hash_table + core_hash_table)
     */
    /*core_dynamic_hash_table_clear(&self->table);*/
}

void core_map_examine(struct core_map *self)
{
    int key_size;
    int value_size;
    uint64_t size;

    key_size = core_map_get_key_size(self);
    value_size = core_map_get_value_size(self);
    size = core_map_size(self);

    printf("core_map_examine key_size %d value_size %d size %" PRIu64 "\n",
                    key_size, value_size, size);
}

struct core_memory_pool *core_map_memory_pool(struct core_map *self)
{
    return core_dynamic_hash_table_memory_pool(&self->table);
}
