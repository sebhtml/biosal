
#ifndef BSAL_SET_HELPER_H
#define BSAL_SET_HELPER_H

struct bsal_set;

int bsal_set_get_any_int(struct bsal_set *self);
void bsal_set_add_int(struct bsal_set *self, int value);

#endif
