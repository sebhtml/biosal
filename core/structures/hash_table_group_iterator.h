
#ifndef BIOSAL_HASH_TABLE_GROUP_ITERATOR_H
#define BIOSAL_HASH_TABLE_GROUP_ITERATOR_H

#include "hash_table_group.h"

#include <stdint.h>

struct biosal_hash_table_group_iterator {
    struct biosal_hash_table_group *list;
    int64_t index;
    int size;
    int key_size;
    int value_size;
};

void biosal_hash_table_group_iterator_init(struct biosal_hash_table_group_iterator *self, struct biosal_hash_table_group *list,
                int size, int key_size, int value_size);
void biosal_hash_table_group_iterator_destroy(struct biosal_hash_table_group_iterator *self);

int biosal_hash_table_group_iterator_has_next(struct biosal_hash_table_group_iterator *self);
void biosal_hash_table_group_iterator_next(struct biosal_hash_table_group_iterator *self, void **key, void **value);

#endif
