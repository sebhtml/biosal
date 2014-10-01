
#ifndef CORE_RED_BLACK_TREE_ITERATOR_H
#define CORE_RED_BLACK_TREE_ITERATOR_H

#include <core/structures/stack.h>

struct core_red_black_tree;

struct core_red_black_tree_iterator {
    struct core_stack stack;
    struct core_red_black_tree *tree;
};

void core_red_black_tree_iterator_init(struct core_red_black_tree_iterator *self,
                struct core_red_black_tree *tree);
void core_red_black_tree_iterator_destroy(struct core_red_black_tree_iterator *self);

int core_red_black_tree_iterator_has_next(struct core_red_black_tree_iterator *self);
int core_red_black_tree_iterator_next(struct core_red_black_tree_iterator *self, void **key, void **value);
int core_red_black_tree_iterator_get_next_key_and_value(struct core_red_black_tree_iterator *self, void *key, void *value);

#endif
