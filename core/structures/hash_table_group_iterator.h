
#ifndef CORE_HASH_TABLE_GROUP_ITERATOR_H
#define CORE_HASH_TABLE_GROUP_ITERATOR_H

#include "hash_table_group.h"

#include <stdint.h>

struct core_hash_table_group_iterator {
    struct core_hash_table_group *list;
    int64_t index;
    int size;
    int key_size;
    int value_size;
};

void core_hash_table_group_iterator_init(struct core_hash_table_group_iterator *self, struct core_hash_table_group *list,
                int size, int key_size, int value_size);
void core_hash_table_group_iterator_destroy(struct core_hash_table_group_iterator *self);

int core_hash_table_group_iterator_has_next(struct core_hash_table_group_iterator *self);
void core_hash_table_group_iterator_next(struct core_hash_table_group_iterator *self, void **key, void **value);

#endif
