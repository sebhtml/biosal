
#ifndef BIOSAL_SET_HELPER_H
#define BIOSAL_SET_HELPER_H

struct biosal_set;

int biosal_set_get_any_int(struct biosal_set *self);
void biosal_set_add_int(struct biosal_set *self, int value);

#endif
