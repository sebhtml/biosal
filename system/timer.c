
#include "timer.h"

#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

void bsal_timer_init(struct bsal_timer *self)
{
    self->start = 0;
}

void bsal_timer_destroy(struct bsal_timer *self)
{
    self->start = 0;
}

void bsal_timer_start(struct bsal_timer *self)
{
    self->start = bsal_timer_get_nanoseconds_from_clock(self);
}

void bsal_timer_stop(struct bsal_timer *self)
{

    self->stop = bsal_timer_get_nanoseconds_from_clock(self);

#ifdef BSAL_TIMER_DEBUG
    printf("TIMER start %" PRIu64 " stop %" PRIu64 "\n", stop, self->start);
#endif

}

/*
 * \see http://pubs.opengroup.org/onlinepubs/7908799/xsh/clock_settime.html
 * \see http://linux.die.net/man/3/clock_gettime
 * \see http://stackoverflow.com/questions/3523442/difference-between-clock-realtime-and-clock-monotonic
 */
uint64_t bsal_timer_get_nanoseconds_from_clock(struct bsal_timer *self)
{
    uint64_t value;
    struct timespec time_value;
    clockid_t clock;

    clock = CLOCK_MONOTONIC;

    clock_gettime(clock, &time_value);

    value = (uint64_t)time_value.tv_sec * 1000000000 + (uint64_t)time_value.tv_nsec;

    return value;
}

void bsal_timer_print(struct bsal_timer *self)
{
    uint64_t nanoseconds;
    float microseconds;

    nanoseconds = bsal_timer_get_elapsed_nanoseconds(self);
    microseconds = nanoseconds / 1000.0;

    printf("TIMER ELAPSED TIME: %f microseconds\n",
                    microseconds);
}

uint64_t bsal_timer_get_elapsed_nanoseconds(struct bsal_timer *self)
{
    return self->stop - self->start;
}
