
#ifndef BSAL_HASH_TABLE_ITERATOR_H
#define BSAL_HASH_TABLE_ITERATOR_H

#include "hash_table.h"

struct bsal_hash_table_iterator {
    struct bsal_hash_table *list;
    uint64_t index;
};

void bsal_hash_table_iterator_init(struct bsal_hash_table_iterator *self, struct bsal_hash_table *list);
void bsal_hash_table_iterator_destroy(struct bsal_hash_table_iterator *self);

int bsal_hash_table_iterator_has_next(struct bsal_hash_table_iterator *self);
void bsal_hash_table_iterator_next(struct bsal_hash_table_iterator *self, void **key, void **value);

#endif
