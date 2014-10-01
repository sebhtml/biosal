
#ifndef BIOSAL_DYNAMIC_HASH_TABLE_ITERATOR_H
#define BIOSAL_DYNAMIC_HASH_TABLE_ITERATOR_H

#include "dynamic_hash_table.h"

struct biosal_dynamic_hash_table_iterator {
    struct biosal_dynamic_hash_table *list;
    uint64_t index;
};

void biosal_dynamic_hash_table_iterator_init(struct biosal_dynamic_hash_table_iterator *self, struct biosal_dynamic_hash_table *list);
void biosal_dynamic_hash_table_iterator_destroy(struct biosal_dynamic_hash_table_iterator *self);

int biosal_dynamic_hash_table_iterator_has_next(struct biosal_dynamic_hash_table_iterator *self);
int biosal_dynamic_hash_table_iterator_next(struct biosal_dynamic_hash_table_iterator *self, void **key, void **value);

#endif
