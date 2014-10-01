
#include "pair.h"

void biosal_pair_init(struct biosal_pair *pair, int first, int second)
{
    pair->first = first;
    pair->second = second;
}

void biosal_pair_destroy(struct biosal_pair *pair)
{
    pair->first = -1;
    pair->second = -1;
}

int biosal_pair_get_first(struct biosal_pair *pair)
{
    return pair->first;
}

int biosal_pair_get_second(struct biosal_pair *pair)
{
    return pair->second;
}

void biosal_pair_set_first(struct biosal_pair *pair, int value)
{
    pair->first = value;
}

void biosal_pair_set_second(struct biosal_pair *pair, int value)
{
    pair->second = value;
}
