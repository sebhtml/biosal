
#ifndef BIOSAL_HASH_TABLE_ITERATOR_H
#define BIOSAL_HASH_TABLE_ITERATOR_H

#include "hash_table.h"

struct biosal_hash_table_iterator {
    struct biosal_hash_table *list;
    uint64_t index;
};

void biosal_hash_table_iterator_init(struct biosal_hash_table_iterator *self, struct biosal_hash_table *list);
void biosal_hash_table_iterator_destroy(struct biosal_hash_table_iterator *self);

int biosal_hash_table_iterator_has_next(struct biosal_hash_table_iterator *self);
void biosal_hash_table_iterator_next(struct biosal_hash_table_iterator *self, void **key, void **value);

#endif
