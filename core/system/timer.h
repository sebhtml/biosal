
#ifndef BSAL_TIMER_H
#define BSAL_TIMER_H

#include <stdint.h>

struct bsal_timer {
    uint64_t start;
    uint64_t stop;
    int started;
    int stopped;

    double frequency;
};

void bsal_timer_init(struct bsal_timer *self);
void bsal_timer_destroy(struct bsal_timer *self);

void bsal_timer_start(struct bsal_timer *self);
void bsal_timer_stop(struct bsal_timer *self);

uint64_t bsal_timer_get_nanoseconds(struct bsal_timer *self);
uint64_t bsal_timer_get_elapsed_nanoseconds(struct bsal_timer *self);

void bsal_timer_print_with_description(struct bsal_timer *self, const char *description);

uint64_t bsal_timer_get_nanoseconds_clock_gettime(struct bsal_timer *self);
uint64_t bsal_timer_get_nanoseconds_gettimeofday(struct bsal_timer *self);
uint64_t bsal_timer_get_nanoseconds_blue_gene_q(struct bsal_timer *self);
uint64_t bsal_timer_get_nanoseconds_apple(struct bsal_timer *self);
double bsal_timer_fetch_frequency(struct bsal_timer *self);

#endif
