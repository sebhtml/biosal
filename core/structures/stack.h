
#ifndef CORE_STACK_H
#define CORE_STACK_H

#include <core/structures/vector.h>

/*
 * A stack (pop / push)
 */
struct core_stack {
    struct core_vector vector;
};

void core_stack_init(struct core_stack *self, int value_size);
void core_stack_destroy(struct core_stack *self);

int core_stack_pop(struct core_stack *self, void *value);
int core_stack_push(struct core_stack *self, void *value);
int core_stack_size(struct core_stack *self);
int core_stack_empty(struct core_stack *self);
int core_stack_full(struct core_stack *self);

#endif
