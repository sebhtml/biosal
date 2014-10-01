
#include "pair.h"

void core_pair_init(struct core_pair *pair, int first, int second)
{
    pair->first = first;
    pair->second = second;
}

void core_pair_destroy(struct core_pair *pair)
{
    pair->first = -1;
    pair->second = -1;
}

int core_pair_get_first(struct core_pair *pair)
{
    return pair->first;
}

int core_pair_get_second(struct core_pair *pair)
{
    return pair->second;
}

void core_pair_set_first(struct core_pair *pair, int value)
{
    pair->first = value;
}

void core_pair_set_second(struct core_pair *pair, int value)
{
    pair->second = value;
}
