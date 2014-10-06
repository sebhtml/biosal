
#ifndef CORE_TIMER_H
#define CORE_TIMER_H

#include <stdint.h>

struct core_timer {
    uint64_t start;
    uint64_t stop;
    int started;
    int stopped;

    double frequency;
};

void core_timer_init(struct core_timer *self);
void core_timer_destroy(struct core_timer *self);

void core_timer_start(struct core_timer *self);
void core_timer_stop(struct core_timer *self);

uint64_t core_timer_get_nanoseconds(struct core_timer *self);
uint64_t core_timer_get_elapsed_nanoseconds(struct core_timer *self);

void core_timer_print_with_description(struct core_timer *self, const char *description);

#endif
