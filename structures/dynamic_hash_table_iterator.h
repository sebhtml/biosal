
#ifndef BSAL_DYNAMIC_HASH_TABLE_ITERATOR_H
#define BSAL_DYNAMIC_HASH_TABLE_ITERATOR_H

#include "dynamic_hash_table.h"

struct bsal_dynamic_hash_table_iterator {
    struct bsal_dynamic_hash_table *list;
    int index;
};

void bsal_dynamic_hash_table_iterator_init(struct bsal_dynamic_hash_table_iterator *self, struct bsal_dynamic_hash_table *list);
void bsal_dynamic_hash_table_iterator_destroy(struct bsal_dynamic_hash_table_iterator *self);

int bsal_dynamic_hash_table_iterator_has_next(struct bsal_dynamic_hash_table_iterator *self);
void bsal_dynamic_hash_table_iterator_next(struct bsal_dynamic_hash_table_iterator *self, void **key, void **value);

#endif
