
#ifndef BIOSAL_TIMER_H
#define BIOSAL_TIMER_H

#include <stdint.h>

struct biosal_timer {
    uint64_t start;
    uint64_t stop;
    int started;
    int stopped;

    double frequency;
};

void biosal_timer_init(struct biosal_timer *self);
void biosal_timer_destroy(struct biosal_timer *self);

void biosal_timer_start(struct biosal_timer *self);
void biosal_timer_stop(struct biosal_timer *self);

uint64_t biosal_timer_get_nanoseconds(struct biosal_timer *self);
uint64_t biosal_timer_get_elapsed_nanoseconds(struct biosal_timer *self);

void biosal_timer_print_with_description(struct biosal_timer *self, const char *description);

uint64_t biosal_timer_get_nanoseconds_clock_gettime(struct biosal_timer *self);
uint64_t biosal_timer_get_nanoseconds_gettimeofday(struct biosal_timer *self);
uint64_t biosal_timer_get_nanoseconds_blue_gene_q(struct biosal_timer *self);
uint64_t biosal_timer_get_nanoseconds_apple(struct biosal_timer *self);
double biosal_timer_fetch_frequency(struct biosal_timer *self);

#endif
