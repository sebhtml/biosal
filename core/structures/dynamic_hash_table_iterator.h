
#ifndef CORE_DYNAMIC_HASH_TABLE_ITERATOR_H
#define CORE_DYNAMIC_HASH_TABLE_ITERATOR_H

#include "dynamic_hash_table.h"

struct core_dynamic_hash_table_iterator {
    struct core_dynamic_hash_table *list;
    uint64_t index;
};

void core_dynamic_hash_table_iterator_init(struct core_dynamic_hash_table_iterator *self, struct core_dynamic_hash_table *list);
void core_dynamic_hash_table_iterator_destroy(struct core_dynamic_hash_table_iterator *self);

int core_dynamic_hash_table_iterator_has_next(struct core_dynamic_hash_table_iterator *self);
int core_dynamic_hash_table_iterator_next(struct core_dynamic_hash_table_iterator *self, void **key, void **value);

#endif
