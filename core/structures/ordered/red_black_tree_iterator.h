
#ifndef BSAL_RED_BLACK_TREE_ITERATOR_H
#define BSAL_RED_BLACK_TREE_ITERATOR_H

#include <core/structures/stack.h>

struct bsal_red_black_tree;

struct bsal_red_black_tree_iterator {
    struct bsal_stack stack;
    struct bsal_red_black_tree *tree;
};

void bsal_red_black_tree_iterator_init(struct bsal_red_black_tree_iterator *self,
                struct bsal_red_black_tree *tree);
void bsal_red_black_tree_iterator_destroy(struct bsal_red_black_tree_iterator *self);

int bsal_red_black_tree_iterator_has_next(struct bsal_red_black_tree_iterator *self);
int bsal_red_black_tree_iterator_next(struct bsal_red_black_tree_iterator *self, void **key, void **value);
int bsal_red_black_tree_iterator_get_next_key_and_value(struct bsal_red_black_tree_iterator *self, void *key, void *value);

#endif
