
#include "set.h"

#include <stdlib.h>

void core_set_init(struct core_set *self, int key_size)
{
    /*
     * use 0 for value size...
     */
    core_map_init(&self->map, key_size, 0);
}

void core_set_destroy(struct core_set *self)
{
    core_map_destroy(&self->map);
}

int core_set_add(struct core_set *self, void *key)
{
    /* It is already in the set
     */
    if (core_set_find(self, key)) {
        return 0;
    }

    core_map_add(&self->map, key);

    return 1;
}

int core_set_find(struct core_set *self, void *key)
{
    void *value;

    value = core_map_get(&self->map, key);

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

int core_set_delete(struct core_set *self, void *key)
{
    if (!core_set_find(self, key)) {
        return 0;
    }

    core_map_delete(&self->map, key);

    return 1;
}

uint64_t core_set_size(struct core_set *self)
{
    return core_map_size(&self->map);
}

struct core_map *core_set_map(struct core_set *self)
{
    return &self->map;
}

int core_set_empty(struct core_set *self)
{
    return core_set_size(self) == 0;
}

void core_set_set_memory_pool(struct core_set *self, struct core_memory_pool *pool)
{
    core_map_set_memory_pool(&self->map, pool);
}

void core_set_clear(struct core_set *self)
{
    core_map_clear(&self->map);
}
