
#include "tracepoint_session.h"

#include <core/structures/string.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#define NO_VALUE 0

void thorium_tracepoint_session_analyze_latency(struct thorium_tracepoint_session *self);

void thorium_tracepoint_session_init(struct thorium_tracepoint_session *self)
{
    core_vector_init(&self->names, sizeof(struct core_string));
    core_vector_init(&self->values, sizeof(uint64_t));
}

void thorium_tracepoint_session_destroy(struct thorium_tracepoint_session *self)
{
    int i;
    int size;
    struct core_string *string_object;

    size = core_vector_size(&self->names);

    for (i = 0; i < size; ++i) {
        string_object = core_vector_at(&self->names, i);

        core_string_destroy(string_object);
    }

    core_vector_destroy(&self->names);
    core_vector_destroy(&self->values);
}

void thorium_tracepoint_session_add_tracepoint(struct thorium_tracepoint_session *self,
                int tracepoint, const char *name)
{
    struct core_string name_string;
    uint64_t time;

    core_string_init(&name_string, name);

    core_vector_push_back(&self->names, &name_string);

    time = NO_VALUE;

    core_vector_push_back(&self->values, &time);

#ifdef TRACEPOINT_VERBOSE
    printf("add tracepoint %d %s\n", tracepoint, name);
#endif
}
void thorium_tracepoint_session_set_tracepoint_time(struct thorium_tracepoint_session *self,
                int tracepoint, uint64_t time)
{
    int size;

    size = core_vector_size(&self->names);

    if (tracepoint >= 0 && tracepoint < size) {

#ifdef TRACEPOINT_VERBOSE
        printf("set tracepoint %d %" PRIu64 "\n", tracepoint, time);
#endif

        core_vector_set(&self->values, tracepoint, &time);

        if (tracepoint == size - 1) {

            thorium_tracepoint_session_analyze_latency(self);
        }
    }
}
uint64_t thorium_tracepoint_session_get_tracepoint_time(struct thorium_tracepoint_session *self,
                int tracepoint)
{
    int size;

    size = core_vector_size(&self->names);

    if (tracepoint >= 0 && tracepoint < size)
        return core_vector_at_as_uint64_t(&self->values, tracepoint);

    return NO_VALUE;
}

void thorium_tracepoint_session_analyze_latency(struct thorium_tracepoint_session *self)
{
    uint64_t current;
    uint64_t previous;
    char *name;
    int i;
    int size;
    struct core_string *string_object;
    uint64_t difference;
    int threshold;
    int latency;
    uint64_t first;
    uint64_t last;

    size = core_vector_size(&self->names);

    /*
     * Threshold is 500 us
     */
    threshold = 500 * 1000;
    first = thorium_tracepoint_session_get_tracepoint_time(self, 0);
    last = thorium_tracepoint_session_get_tracepoint_time(self, size - 1);
    latency = last - first;

    /*
    printf("latency %d\n", latency);
    */
    if (latency < threshold)
        return;

    previous = NO_VALUE;
    i = 0;

    printf("tracepoint session (granularity: %d ns)\n", latency);

    while (i < size) {
        current = thorium_tracepoint_session_get_tracepoint_time(self, i);
        string_object = core_vector_at(&self->names, i);
        name = core_string_get(string_object);

        if (current == NO_VALUE) {
            printf("tracepoint %s time -\n", name);

        } else if (current != NO_VALUE && previous == NO_VALUE) {
            printf("tracepoint %s time %" PRIu64 " ns\n", name, current);

        } else if (current != NO_VALUE && previous != NO_VALUE) {

            difference = current - previous;
            printf("tracepoint %s time %" PRIu64 " ns (+ %" PRIu64 " ns)\n", name, current,
                            difference);
        }

        if (current != NO_VALUE)
            previous = current;

        ++i;
    }

    printf("\n");
}
