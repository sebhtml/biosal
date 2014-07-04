
#ifndef BSAL_PAIR_H
#define BSAL_PAIR_H

struct bsal_pair {
    int first;
    int second;
};

void bsal_pair_init(struct bsal_pair *pair, int first, int second);
void bsal_pair_destroy(struct bsal_pair *pair);
int bsal_pair_get_first(struct bsal_pair *pair);
int bsal_pair_get_second(struct bsal_pair *pair);

#endif
