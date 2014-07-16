
#ifndef BSAL_TIMER_H
#define BSAL_TIMER_H

#include <stdint.h>

struct bsal_timer {
    uint64_t start;
    uint64_t stop;
    int started;
    int stopped;
};

void bsal_timer_init(struct bsal_timer *self);
void bsal_timer_destroy(struct bsal_timer *self);

void bsal_timer_start(struct bsal_timer *self);
void bsal_timer_stop(struct bsal_timer *self);

uint64_t bsal_timer_get_nanoseconds();
uint64_t bsal_timer_get_elapsed_nanoseconds(struct bsal_timer *self);

void bsal_timer_print(struct bsal_timer *self);
void bsal_timer_print_with_description(struct bsal_timer *self, const char *description);

uint64_t bsal_timer_get_nanoseconds_clock_gettime();
uint64_t bsal_timer_get_nanoseconds_gettimeofday();

#endif
