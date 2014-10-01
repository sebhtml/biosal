
#ifndef CORE_SET_HELPER_H
#define CORE_SET_HELPER_H

struct core_set;

int core_set_get_any_int(struct core_set *self);
void core_set_add_int(struct core_set *self, int value);

#endif
