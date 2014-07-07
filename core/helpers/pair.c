
#include "pair.h"

void bsal_pair_init(struct bsal_pair *pair, int first, int second)
{
    pair->first = first;
    pair->second = second;
}

void bsal_pair_destroy(struct bsal_pair *pair)
{
    pair->first = -1;
    pair->second = -1;
}

int bsal_pair_get_first(struct bsal_pair *pair)
{
    return pair->first;
}

int bsal_pair_get_second(struct bsal_pair *pair)
{
    return pair->second;
}

void bsal_pair_set_first(struct bsal_pair *pair, int value)
{
    pair->first = value;
}

void bsal_pair_set_second(struct bsal_pair *pair, int value)
{
    pair->second = value;
}
