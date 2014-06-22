
#include "map.h"

#include <stdio.h>
#include <string.h>

void bsal_map_init(struct bsal_map *self, int key_size, int value_size)
{
    uint64_t buckets = 2;

    bsal_dynamic_hash_table_init(&self->table, buckets, key_size, value_size);
}

void bsal_map_destroy(struct bsal_map *self)
{
    bsal_dynamic_hash_table_destroy(&self->table);
}

void *bsal_map_add(struct bsal_map *self, void *key)
{
    return bsal_dynamic_hash_table_add(&self->table, key);
}

void *bsal_map_get(struct bsal_map *self, void *key)
{
    return bsal_dynamic_hash_table_get(&self->table, key);
}

void bsal_map_delete(struct bsal_map *self, void *key)
{
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
    return bsal_dynamic_hash_table_pack_size(&self->table);
}

int bsal_map_pack(struct bsal_map *self, void *buffer)
{
    return bsal_dynamic_hash_table_pack(&self->table, buffer);
}

int bsal_map_unpack(struct bsal_map *self, void *buffer)
{
    return bsal_dynamic_hash_table_unpack(&self->table, buffer);
}

void bsal_map_add_value(struct bsal_map *self, void *key, void *value)
{
    int value_size;
    void *bucket;

    value_size = bsal_map_get_value_size(self);
    bucket = bsal_map_add(self, key);

    memcpy(bucket, value, value_size);
}

int bsal_map_get_key_size(struct bsal_map *self)
{
    return bsal_dynamic_hash_table_get_key_size(&self->table);
}

int bsal_map_get_value_size(struct bsal_map *self)
{
    return bsal_dynamic_hash_table_get_value_size(&self->table);
}
