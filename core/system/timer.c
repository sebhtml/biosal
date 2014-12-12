
#include "timer.h"

#include <time.h>

#include <sys/time.h>

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#if defined(__bgq__)
#include <spi/include/kernel/location.h>
#endif

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifndef CONFIG_CLOCK_GETTIME
#define CORE_DISABLE_CLOCK_GETTIME
#endif

/*
 * clock_gettime is defective on IBM Blue Gene/Q according
 * to the tests that we did.
 *
 * \see http://sourceforge.net/p/x10/mailman/message/32206231/
 */
#if defined(__bgp__) || defined(__bgq__) || defined(__bg__)
#define CORE_DISABLE_CLOCK_GETTIME
#endif

/*
 * Disable clock_gettime on Mac
 */
#ifdef __APPLE__
#define CORE_DISABLE_CLOCK_GETTIME
#endif


#define NANOSECONDS_IN_MICROSECOND 1000
#define NANOSECONDS_IN_MILLISECOND (1000 * 1000)
#define NANOSECONDS_IN_SECOND (1000 * 1000 * 1000)
#define SECONDS_IN_MINUTE 60

uint64_t core_timer_get_nanoseconds_clock_gettime(struct core_timer *self);
uint64_t core_timer_get_nanoseconds_gettimeofday(struct core_timer *self);
uint64_t core_timer_get_nanoseconds_blue_gene_q(struct core_timer *self);
uint64_t core_timer_get_nanoseconds_apple(struct core_timer *self);

double core_timer_fetch_frequency(struct core_timer *self);

void core_timer_init(struct core_timer *timer)
{
    timer->start = 0;

    timer->started = 0;
    timer->stopped = 0;

    timer->frequency = core_timer_fetch_frequency(timer);
}

void core_timer_destroy(struct core_timer *timer)
{
    timer->start = 0;
}

void core_timer_start(struct core_timer *timer)
{
    timer->start = core_timer_get_nanoseconds(timer);
    timer->started = 1;
}

void core_timer_stop(struct core_timer *timer)
{

    timer->stop = core_timer_get_nanoseconds(timer);

    timer->stopped = 1;

#ifdef CORE_TIMER_DEBUG
    fprintf(stderr, "TIMER start %" PRIu64 " stop %" PRIu64 "\n", stop, timer->start);
#endif

}

uint64_t core_timer_get_nanoseconds(struct core_timer *timer)
{
#if defined(__bgq__)
    return core_timer_get_nanoseconds_blue_gene_q(timer);
#elif defined(__APPLE__)
    return core_timer_get_nanoseconds_apple(timer);

#elif defined(CORE_DISABLE_CLOCK_GETTIME)
    return core_timer_get_nanoseconds_gettimeofday(timer);
#else
    return core_timer_get_nanoseconds_clock_gettime(timer);
#endif
}

uint64_t core_timer_get_nanoseconds_blue_gene_q(struct core_timer *timer)
{
    uint64_t cycles;
    uint64_t picoseconds_per_cycle;
    uint64_t nanoseconds;

    cycles = 0;

#ifdef __bgq__
    cycles = GetTimeBase();
#endif

    /*
     * Get the number of picoseconds per cycle.
     */
    picoseconds_per_cycle = 1;
    picoseconds_per_cycle *= 1000;
    picoseconds_per_cycle *= 1000;
    picoseconds_per_cycle *= 1000;
    picoseconds_per_cycle *= 1000;

    picoseconds_per_cycle /= timer->frequency;

    nanoseconds = (cycles * picoseconds_per_cycle) / 1000;

    return nanoseconds;
}

uint64_t core_timer_get_nanoseconds_gettimeofday(struct core_timer *timer)
{
    uint64_t value;
    struct timeval time;

    gettimeofday(&time, NULL);

    value = (uint64_t)time.tv_sec * 1000000000 + (uint64_t)time.tv_usec * 1000;

    return value;
}

/*
 * \see http://pubs.opengroup.org/onlinepubs/7908799/xsh/clock_settime.html
 * \see http://linux.die.net/man/3/clock_gettime
 * \see http://stackoverflow.com/questions/3523442/difference-between-clock-realtime-and-clock-monotonic
 */
uint64_t core_timer_get_nanoseconds_clock_gettime(struct core_timer *timer)
{
    uint64_t value;

    value = 0;

#ifndef CORE_DISABLE_CLOCK_GETTIME
    struct timespec time_value;
    clockid_t clock;

    clock = CLOCK_MONOTONIC;

    clock_gettime(clock, &time_value);

    value = (uint64_t)time_value.tv_sec * 1000000000 + (uint64_t)time_value.tv_nsec;
#endif

    return value;
}

/*
 * \see http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
 */
uint64_t core_timer_get_nanoseconds_apple(struct core_timer *timer)
{
    uint64_t value;

    value = 0;

#ifdef __APPLE__
    clock_serv_t cclock;
    mach_timespec_t mts;

    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);

    value = (uint64_t)mts.tv_sec * 1000000000 + (uint64_t)mts.tv_nsec;
#endif

    return value;
}

uint64_t core_timer_get_elapsed_nanoseconds(struct core_timer *timer)
{
    return timer->stop - timer->start;
}

void core_timer_print_with_description(struct core_timer *timer, const char *description)
{
    uint64_t nanoseconds;
    float microseconds;
    float milliseconds;
    int seconds;
    float float_seconds;
    int minutes;

    fprintf(stderr, "TIMER [%s] ", description);

    nanoseconds = core_timer_get_elapsed_nanoseconds(timer);

    if (!timer->started || !timer->stopped) {
        fprintf(stderr, "timer error\n");
        return;
    }

    /* Show nanoseconds
     */
    if (nanoseconds < NANOSECONDS_IN_MICROSECOND) {
        fprintf(stderr, "%d nanoseconds\n",
                    (int)nanoseconds);

    /* Show microseconds
     */
    } else if (nanoseconds < NANOSECONDS_IN_MILLISECOND) {
        microseconds = (0.0 + nanoseconds) / NANOSECONDS_IN_MICROSECOND;

        fprintf(stderr, "%f microseconds\n",
                    microseconds);

    /* Show milliseconds
     */
    } else if (nanoseconds < NANOSECONDS_IN_SECOND) {
        milliseconds = (0.0 + nanoseconds) / NANOSECONDS_IN_MILLISECOND;

        fprintf(stderr, "%f milliseconds\n",
                    milliseconds);

    /* Show minutes and seconds
     */
    } else {

        /* example: 121.2 s */
        float_seconds = (nanoseconds + 0.0) / NANOSECONDS_IN_SECOND;

        /* example: 121 s */
        seconds = float_seconds;

        /* example: 2 min */
        minutes = seconds / SECONDS_IN_MINUTE;

        /* example: 1.2s */
        float_seconds -= minutes * SECONDS_IN_MINUTE;

        if (minutes != 0) {
            fprintf(stderr, "%d minutes", minutes);
        }

        if (seconds != 0) {
            if (minutes != 0) {
                fprintf(stderr, ", ");
            }

            fprintf(stderr, "%f seconds", float_seconds);
        }

        fprintf(stderr, "\n");
    }
}

double core_timer_fetch_frequency(struct core_timer *timer)
{
    double frequency;

#ifdef __bgq__
    Personality_t personality;
#endif

    frequency = 1;

    /*
     * \see https://hpc-forge.cineca.it/files/ScuolaCalcoloParallelo_WebDAV/public/anno-2013/advanced-school/adv_mpi_handson.pdf
     * \see https://github.com/GeneAssembly/biosal/issues/429
     */

#ifdef __bgq__
    Kernel_GetPersonality(&personality, sizeof(Personality_t));
    frequency = personality.Kernel_Config.FreqMHz * 1000000.0;
#endif

    return frequency;
}
