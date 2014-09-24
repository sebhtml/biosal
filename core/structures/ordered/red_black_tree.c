
#include "red_black_tree.h"
#include "red_black_node.h"

#include <core/system/memory_pool.h>
#include <core/system/debugger.h>

#include <stdlib.h>

void bsal_red_black_tree_init(struct bsal_red_black_tree *self)
{
    self->root = NULL;
    self->memory_pool = NULL;
    self->size = 0;
}

void bsal_red_black_tree_destroy(struct bsal_red_black_tree *self)
{
    if (self->root != NULL) {
        bsal_red_black_tree_free_node(self, self->root);
        self->root = NULL;
    }

    self->memory_pool = NULL;
    self->size = 0;
}

void bsal_red_black_tree_add(struct bsal_red_black_tree *self, int key)
{
    struct bsal_red_black_node *node;
    struct bsal_red_black_node *current_node;
    struct bsal_red_black_node *left_node;
    struct bsal_red_black_node *right_node;

    node = bsal_memory_pool_allocate(self->memory_pool, sizeof(struct bsal_red_black_node));

    if (self->root == NULL) {
        self->root = node;
        ++self->size;
        return;
    }

    current_node = self->root;

    while (1) {

        if (bsal_red_black_node_key(node) < bsal_red_black_node_key(current_node)) {

            left_node = bsal_red_black_node_left_child(current_node);

            if (left_node == NULL) {
                bsal_red_black_node_set_left_child(current_node, node);
                bsal_red_black_node_set_parent(node, current_node);
                ++self->size;
                break;
            } else {
                current_node = left_node;
            }
        } else /* if (bsal_red_black_node_key(node) < bsal_red_black_node_key(current_node)) */ {

            right_node = bsal_red_black_node_right_child(current_node);

            if (right_node == NULL) {
                bsal_red_black_node_set_right_child(current_node, node);
                bsal_red_black_node_set_parent(node, current_node);
                ++self->size;
                break;
            } else {
                current_node = right_node;
            }
        }
    }
}

void bsal_red_black_tree_delete(struct bsal_red_black_tree *self, int key)
{
}

/*
 * A red-black tree.
 *
 * \see http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 *
 * The 5 rules are:
 *
 * 1. A node is red or black.
 * 2. The root is black.
 * 3. All leaf nodes are black.
 * 4. Any red node has 2 black child nodes.
 * 5. Every path from given node to any of its descendent leaf node contains
 *    the same number of black nodes.
 */
void bsal_red_black_tree_check_rules(struct bsal_red_black_tree *self, int key)
{
    if (self->root == NULL) {
        return;
    }

    if (bsal_red_black_node_color(self->root) != BSAL_COLOR_BLACK) {
        printf("Rule 2 is ignored !\n");
        return;
    }
}

void bsal_red_black_tree_set_memory_pool(struct bsal_red_black_tree *self,
                struct bsal_memory_pool *memory_pool)
{
    self->memory_pool = memory_pool;
}

int bsal_red_black_tree_size(struct bsal_red_black_tree *self)
{
    return self->size;
}

void bsal_red_black_tree_free_node(struct bsal_red_black_tree *self,
                struct bsal_red_black_node *node)
{
    struct bsal_red_black_node *left_node;
    struct bsal_red_black_node *right_node;

    if (node == NULL) {
        return;
    }

#if 0
    printf("free_node\n");
#endif

    left_node = bsal_red_black_node_left_child(node);
    right_node = bsal_red_black_node_right_child(node);

    bsal_red_black_tree_free_node(self, left_node);
    bsal_red_black_tree_free_node(self, right_node);

    bsal_red_black_node_destroy(node);

#if 0
    printf("free %p\n", (void *)node);
#endif
    bsal_memory_pool_free(self->memory_pool, node);
}
