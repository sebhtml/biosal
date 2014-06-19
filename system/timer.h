
#ifndef BSAL_TIMER_H
#define BSAL_TIMER_H

#include <stdint.h>

struct bsal_timer {
    uint64_t start;
    uint64_t stop;
};

void bsal_timer_init(struct bsal_timer *self);
void bsal_timer_destroy(struct bsal_timer *self);

void bsal_timer_start(struct bsal_timer *self);
void bsal_timer_stop(struct bsal_timer *self);

uint64_t bsal_timer_get_nanoseconds_from_clock();
uint64_t bsal_timer_get_elapsed_nanoseconds(struct bsal_timer *self);

void bsal_timer_print(struct bsal_timer *self);

#endif
