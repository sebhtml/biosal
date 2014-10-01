
#ifndef CORE_HASH_TABLE_ITERATOR_H
#define CORE_HASH_TABLE_ITERATOR_H

#include "hash_table.h"

struct core_hash_table_iterator {
    struct core_hash_table *list;
    uint64_t index;
};

void core_hash_table_iterator_init(struct core_hash_table_iterator *self, struct core_hash_table *list);
void core_hash_table_iterator_destroy(struct core_hash_table_iterator *self);

int core_hash_table_iterator_has_next(struct core_hash_table_iterator *self);
void core_hash_table_iterator_next(struct core_hash_table_iterator *self, void **key, void **value);

#endif
