
#ifndef BIOSAL_PAIR_H
#define BIOSAL_PAIR_H

struct biosal_pair {
    int first;
    int second;
};

void biosal_pair_init(struct biosal_pair *pair, int first, int second);
void biosal_pair_destroy(struct biosal_pair *pair);
int biosal_pair_get_first(struct biosal_pair *pair);
int biosal_pair_get_second(struct biosal_pair *pair);
void biosal_pair_set_first(struct biosal_pair *pair, int value);
void biosal_pair_set_second(struct biosal_pair *pair, int value);

#endif
