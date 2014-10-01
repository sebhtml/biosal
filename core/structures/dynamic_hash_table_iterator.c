
#include "dynamic_hash_table_iterator.h"

#include <core/system/debugger.h>

#include <stdlib.h>

void biosal_dynamic_hash_table_iterator_init(struct biosal_dynamic_hash_table_iterator *self, struct biosal_dynamic_hash_table *list)
{
    /* the list must remain frozen while it is being iterated.
     */
    biosal_dynamic_hash_table_finish_resizing(list);

    self->index = 0;
    self->list = list;
}

void biosal_dynamic_hash_table_iterator_destroy(struct biosal_dynamic_hash_table_iterator *self)
{
    self->index = 0;
    self->list = NULL;
}

int biosal_dynamic_hash_table_iterator_has_next(struct biosal_dynamic_hash_table_iterator *self)
{
    uint64_t size;

    if (self->list == NULL) {
        return 0;
    }

    size = biosal_dynamic_hash_table_buckets(self->list);

    if (size == 0) {
        return 0;
    }

    while (self->index < size
                    && biosal_dynamic_hash_table_state(self->list, self->index) !=
                    BIOSAL_HASH_TABLE_BUCKET_OCCUPIED) {
        self->index++;
    }

    if (self->index >= size) {
        return 0;
    }

    /* Make sure that the pointed bucket is occupied
     */
    BIOSAL_DEBUGGER_ASSERT(biosal_dynamic_hash_table_state(self->list, self->index) ==
                    BIOSAL_HASH_TABLE_BUCKET_OCCUPIED);

    return 1;
}

int biosal_dynamic_hash_table_iterator_next(struct biosal_dynamic_hash_table_iterator *self, void **key, void **value)
{
    if (!biosal_dynamic_hash_table_iterator_has_next(self)) {
        return 0;
    }

    BIOSAL_DEBUGGER_ASSERT(biosal_dynamic_hash_table_state(self->list, self->index) ==
                    BIOSAL_HASH_TABLE_BUCKET_OCCUPIED);

    if (key != NULL) {
        *key = biosal_dynamic_hash_table_key(self->list, self->index);

#ifdef BIOSAL_DEBUGGER_ASSERT
        BIOSAL_DEBUGGER_ASSERT(*key != NULL);
#endif
    }

    if (value != NULL) {
        *value = biosal_dynamic_hash_table_value(self->list, self->index);

#ifdef BIOSAL_DEBUGGER_ASSERT
        BIOSAL_DEBUGGER_ASSERT(*value != NULL);
#endif
    }

    self->index++;

    return 1;
}


