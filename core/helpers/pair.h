
#ifndef CORE_PAIR_H
#define CORE_PAIR_H

/* 
 * TODO: Consider renaming this to core_pair_int (as it is concrete, not abstract)
 */

struct core_pair {
    int first;
    int second;
};

void core_pair_init(struct core_pair *pair, int first, int second);
void core_pair_destroy(struct core_pair *pair);
int core_pair_get_first(struct core_pair *pair);
int core_pair_get_second(struct core_pair *pair);
void core_pair_set_first(struct core_pair *pair, int value);
void core_pair_set_second(struct core_pair *pair, int value);

#endif
