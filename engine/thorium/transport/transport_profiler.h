
#ifndef BSAL_TRANSPORT_PROFILER_H
#define BSAL_TRANSPORT_PROFILER_H

#include <core/structures/map.h>

struct thorium_message;

/*
 * A profiler for the Thorium transport
 * component.
 */
struct thorium_transport_profiler {
    struct bsal_map buffer_sizes;
    int rank;
};

void thorium_transport_profiler_init(struct thorium_transport_profiler *self);
void thorium_transport_profiler_destroy(struct thorium_transport_profiler *self);
void thorium_transport_profiler_print_report(struct thorium_transport_profiler *self);
void thorium_transport_profiler_send_mock(struct thorium_transport_profiler *self,
                struct thorium_message *message);

#endif
