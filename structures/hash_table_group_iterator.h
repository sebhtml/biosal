
#ifndef _BSAL_HASH_TABLE_GROUP_ITERATOR_H
#define _BSAL_HASH_TABLE_GROUP_ITERATOR_H

#include "hash_table_group.h"

struct bsal_hash_table_group_iterator {
    struct bsal_hash_table_group *list;
    int index;
    int size;
    int key_size;
    int value_size;
};

void bsal_hash_table_group_iterator_init(struct bsal_hash_table_group_iterator *self, struct bsal_hash_table_group *list,
                int size, int key_size, int value_size);
void bsal_hash_table_group_iterator_destroy(struct bsal_hash_table_group_iterator *self);

int bsal_hash_table_group_iterator_has_next(struct bsal_hash_table_group_iterator *self);
void bsal_hash_table_group_iterator_next(struct bsal_hash_table_group_iterator *self, void **key, void **value);

#endif
