
#include "set.h"

#include <stdlib.h>

void biosal_set_init(struct biosal_set *self, int key_size)
{
    /*
     * use 0 for value size...
     */
    biosal_map_init(&self->map, key_size, 0);
}

void biosal_set_destroy(struct biosal_set *self)
{
    biosal_map_destroy(&self->map);
}

int biosal_set_add(struct biosal_set *self, void *key)
{
    /* It is already in the set
     */
    if (biosal_set_find(self, key)) {
        return 0;
    }

    biosal_map_add(&self->map, key);

    return 1;
}

int biosal_set_find(struct biosal_set *self, void *key)
{
    void *value;

    value = biosal_map_get(&self->map, key);

    if (value != NULL) {

        /*
         * don't do anything with this pointer because it point to 0 bytes !
         * In fact, it is a pointer to the next key since the set is using
         * 0 bytes for values (no values)
         */

        return 1;
    }

    return 0;
}

int biosal_set_delete(struct biosal_set *self, void *key)
{
    if (!biosal_set_find(self, key)) {
        return 0;
    }

    biosal_map_delete(&self->map, key);

    return 1;
}

uint64_t biosal_set_size(struct biosal_set *self)
{
    return biosal_map_size(&self->map);
}

struct biosal_map *biosal_set_map(struct biosal_set *self)
{
    return &self->map;
}

int biosal_set_empty(struct biosal_set *self)
{
    return biosal_set_size(self) == 0;
}

void biosal_set_set_memory_pool(struct biosal_set *self, struct biosal_memory_pool *pool)
{
    biosal_map_set_memory_pool(&self->map, pool);
}

void biosal_set_clear(struct biosal_set *self)
{
    biosal_map_clear(&self->map);
}
