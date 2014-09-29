
#ifndef BSAL_STACK_H
#define BSAL_STACK_H

#include <core/structures/vector.h>

/*
 * A stack (pop / push)
 */
struct bsal_stack {
    struct bsal_vector vector;
};

void bsal_stack_init(struct bsal_stack *self, int value_size);
void bsal_stack_destroy(struct bsal_stack *self);
int bsal_stack_pop(struct bsal_stack *self, void *value);
int bsal_stack_push(struct bsal_stack *self, void *value);
int bsal_stack_size(struct bsal_stack *self);
int bsal_stack_empty(struct bsal_stack *self);
int bsal_stack_full(struct bsal_stack *self);

#endif
