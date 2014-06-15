
#include "map.h"

void bsal_map_init(struct bsal_map *self, int key_size, int value_size)
{
    bsal_dynamic_hash_table_init(&self->table, 2, key_size, value_size);
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

uint64_t bsal_map_buckets(struct bsal_map *self)
{
    return bsal_dynamic_hash_table_buckets(&self->table);
}

int bsal_map_state(struct bsal_map *self, uint64_t bucket)
{
    return bsal_dynamic_hash_table_state(&self->table, bucket);
}

void *bsal_map_key(struct bsal_map *self, uint64_t bucket)
{
    return bsal_dynamic_hash_table_key(&self->table, bucket);
}

void *bsal_map_value(struct bsal_map *self, uint64_t bucket)
{
    return bsal_dynamic_hash_table_value(&self->table, bucket);
}


