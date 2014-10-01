
#ifndef BIOSAL_RED_BLACK_TREE_ITERATOR_H
#define BIOSAL_RED_BLACK_TREE_ITERATOR_H

#include <core/structures/stack.h>

struct biosal_red_black_tree;

struct biosal_red_black_tree_iterator {
    struct biosal_stack stack;
    struct biosal_red_black_tree *tree;
};

void biosal_red_black_tree_iterator_init(struct biosal_red_black_tree_iterator *self,
                struct biosal_red_black_tree *tree);
void biosal_red_black_tree_iterator_destroy(struct biosal_red_black_tree_iterator *self);

int biosal_red_black_tree_iterator_has_next(struct biosal_red_black_tree_iterator *self);
int biosal_red_black_tree_iterator_next(struct biosal_red_black_tree_iterator *self, void **key, void **value);
int biosal_red_black_tree_iterator_get_next_key_and_value(struct biosal_red_black_tree_iterator *self, void *key, void *value);

#endif
