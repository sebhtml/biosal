
#include "timer.h"

#include <time.h>

#include <sys/time.h>

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

#if defined(__bgq__)
#include <spi/include/kernel/location.h>
#endif

/*
 * clock_gettime is defective on IBM Blue Gene/Q according
 * to the tests that we did.
 *
 * \see http://sourceforge.net/p/x10/mailman/message/32206231/
 */
#if defined(__bgp__) || defined(__bgq__) || defined(__bg__)
#define BIOSAL_DISABLE_CLOCK_GETTIME
#endif


#define NANOSECONDS_IN_MICROSECOND 1000
#define NANOSECONDS_IN_MILLISECOND (1000 * 1000)
#define NANOSECONDS_IN_SECOND (1000 * 1000 * 1000)
#define SECONDS_IN_MINUTE 60

void bsal_timer_init(struct bsal_timer *timer)
{
    timer->start = 0;

    timer->started = 0;
    timer->stopped = 0;

    timer->frequency = bsal_timer_fetch_frequency(timer);
}

void bsal_timer_destroy(struct bsal_timer *timer)
{
    timer->start = 0;
}

void bsal_timer_start(struct bsal_timer *timer)
{
    timer->start = bsal_timer_get_nanoseconds(timer);
    timer->started = 1;
}

void bsal_timer_stop(struct bsal_timer *timer)
{

    timer->stop = bsal_timer_get_nanoseconds(timer);

    timer->stopped = 1;

#ifdef BSAL_TIMER_DEBUG
    printf("TIMER start %" PRIu64 " stop %" PRIu64 "\n", stop, timer->start);
#endif

}

uint64_t bsal_timer_get_nanoseconds(struct bsal_timer *timer)
{
#if defined(__bgq__)
    return bsal_timer_get_nanoseconds_blue_gene_q(timer);

#elif defined(BIOSAL_DISABLE_CLOCK_GETTIME)
    return bsal_timer_get_nanoseconds_gettimeofday(timer);
#else
    return bsal_timer_get_nanoseconds_clock_gettime(timer);
#endif
}

uint64_t bsal_timer_get_nanoseconds_blue_gene_q(struct bsal_timer *timer)
{
    uint64_t cycles;
    uint64_t time;
    uint64_t microseconds_per_cycle;

    cycles = 0;
    time = 0;

#ifdef __bgq__
    cycles = GetTimeBase();
#endif

    microseconds_per_cycle = (1000 * 1000) / timer->frequency;
    time = cycles * microseconds_per_cycle;

    return time * 1000;
}

uint64_t bsal_timer_get_nanoseconds_gettimeofday(struct bsal_timer *timer)
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
uint64_t bsal_timer_get_nanoseconds_clock_gettime(struct bsal_timer *timer)
{
    uint64_t value;
    struct timespec time_value;
    clockid_t clock;

    clock = CLOCK_MONOTONIC;

    clock_gettime(clock, &time_value);

    value = (uint64_t)time_value.tv_sec * 1000000000 + (uint64_t)time_value.tv_nsec;

    return value;
}

void bsal_timer_print(struct bsal_timer *timer)
{
    uint64_t nanoseconds;
    float microseconds;
    float milliseconds;
    int seconds;
    float float_seconds;
    int minutes;

    nanoseconds = bsal_timer_get_elapsed_nanoseconds(timer);

    if (!timer->started || !timer->stopped) {
        printf("timer error\n");
        return;
    }

    /* Show nanoseconds
     */
    if (nanoseconds < NANOSECONDS_IN_MICROSECOND) {
        printf("%d nanoseconds\n",
                    (int)nanoseconds);

    /* Show microseconds
     */
    } else if (nanoseconds < NANOSECONDS_IN_MILLISECOND) {
        microseconds = (0.0 + nanoseconds) / NANOSECONDS_IN_MICROSECOND;

        printf("%f microseconds\n",
                    microseconds);

    /* Show milliseconds
     */
    } else if (nanoseconds < NANOSECONDS_IN_SECOND) {
        milliseconds = (0.0 + nanoseconds) / NANOSECONDS_IN_MILLISECOND;

        printf("%f milliseconds\n",
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
            printf("%d minutes", minutes);
        }

        if (seconds != 0) {
            if (minutes != 0) {
                printf(", ");
            }

            printf("%f seconds", float_seconds);
        }

        printf("\n");
    }
}

uint64_t bsal_timer_get_elapsed_nanoseconds(struct bsal_timer *timer)
{
    return timer->stop - timer->start;
}

void bsal_timer_print_with_description(struct bsal_timer *timer, const char *description)
{
    printf("TIMER <%s> ", description);

    bsal_timer_print(timer);
}

double bsal_timer_fetch_frequency(struct bsal_timer *timer)
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
