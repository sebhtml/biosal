
#ifndef BIOSAL_STACK_H
#define BIOSAL_STACK_H

#include <core/structures/vector.h>

/*
 * A stack (pop / push)
 */
struct biosal_stack {
    struct biosal_vector vector;
};

void biosal_stack_init(struct biosal_stack *self, int value_size);
void biosal_stack_destroy(struct biosal_stack *self);
int biosal_stack_pop(struct biosal_stack *self, void *value);
int biosal_stack_push(struct biosal_stack *self, void *value);
int biosal_stack_size(struct biosal_stack *self);
int biosal_stack_empty(struct biosal_stack *self);
int biosal_stack_full(struct biosal_stack *self);

#endif
