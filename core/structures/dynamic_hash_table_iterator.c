
#include "dynamic_hash_table_iterator.h"

#include <core/system/debugger.h>

#include <stdlib.h>

void core_dynamic_hash_table_iterator_init(struct core_dynamic_hash_table_iterator *self, struct core_dynamic_hash_table *list)
{
    /* the list must remain frozen while it is being iterated.
     */
    core_dynamic_hash_table_finish_resizing(list);

    self->index = 0;
    self->list = list;
}

void core_dynamic_hash_table_iterator_destroy(struct core_dynamic_hash_table_iterator *self)
{
    self->index = 0;
    self->list = NULL;
}

int core_dynamic_hash_table_iterator_has_next(struct core_dynamic_hash_table_iterator *self)
{
    uint64_t size;

    if (self->list == NULL) {
        return 0;
    }

    size = core_dynamic_hash_table_buckets(self->list);

    if (size == 0) {
        return 0;
    }

    while (self->index < size
                    && core_dynamic_hash_table_state(self->list, self->index) !=
                    CORE_HASH_TABLE_BUCKET_OCCUPIED) {
        self->index++;
    }

    if (self->index >= size) {
        return 0;
    }

    /* Make sure that the pointed bucket is occupied
     */
    CORE_DEBUGGER_ASSERT(core_dynamic_hash_table_state(self->list, self->index) ==
                    CORE_HASH_TABLE_BUCKET_OCCUPIED);

    return 1;
}

int core_dynamic_hash_table_iterator_next(struct core_dynamic_hash_table_iterator *self, void **key, void **value)
{
    if (!core_dynamic_hash_table_iterator_has_next(self)) {
        return 0;
    }

    CORE_DEBUGGER_ASSERT(core_dynamic_hash_table_state(self->list, self->index) ==
                    CORE_HASH_TABLE_BUCKET_OCCUPIED);

    if (key != NULL) {
        *key = core_dynamic_hash_table_key(self->list, self->index);

#ifdef CORE_DEBUGGER_ASSERT
        CORE_DEBUGGER_ASSERT(*key != NULL);
#endif
    }

    if (value != NULL) {
        *value = core_dynamic_hash_table_value(self->list, self->index);

#ifdef CORE_DEBUGGER_ASSERT
        CORE_DEBUGGER_ASSERT(*value != NULL);
#endif
    }

    self->index++;

    return 1;
}


