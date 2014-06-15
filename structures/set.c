
#include "set.h"

#include <stdlib.h>

void bsal_set_init(struct bsal_set *self, int key_size)
{
    /* TODO
     * use 0 for value size...
     */
    bsal_map_init(&self->map, key_size, 4);
}

void bsal_set_destroy(struct bsal_set *self)
{
    bsal_map_destroy(&self->map);
}

void bsal_set_add(struct bsal_set *self, void *key)
{
    bsal_map_add(&self->map, key);
}

int bsal_set_find(struct bsal_set *self, void *key)
{
    if (bsal_map_get(&self->map, key) != NULL) {
        return 1;
    }

    return 0;
}

void bsal_set_delete(struct bsal_set *self, void *key)
{
    bsal_map_delete(&self->map, key);
}

uint64_t bsal_set_size(struct bsal_set *self)
{
    return bsal_map_size(&self->map);
}

struct bsal_map *bsal_set_map(struct bsal_set *self)
{
    return &self->map;
}
