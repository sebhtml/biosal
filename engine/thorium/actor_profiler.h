#ifndef THORIUM_ACTOR_PROFILER_H
#define THORIUM_ACTOR_PROFILER_H

#define THORIUM_ACTOR_PROFILER_RECEIVE_BEGIN   0
#define THORIUM_ACTOR_PROFILER_RECEIVE_END     1

#define THORIUM_ACTOR_PROFILER_HEADER \
    "start_time\tend_time\tactor\tscript\taction\tcount\tsource\tcommunication_time\tcompute_time\tcompute_to_communication_ratio\n"

#include <core/structures/vector.h>

#include <core/system/timer.h>

#include "message.h"

#include <inttypes.h>
#include <stdint.h>

struct core_buffered_file_writer;

/*
 * Event profiler.
 */
struct thorium_actor_profiler {
    uint64_t profile_begin_count;
    uint64_t profile_end_count;

    struct core_timer timer;
    struct core_vector event_start_times;
    struct core_vector event_end_times;
    struct core_vector event_actions;
    struct core_vector event_counts;
    struct core_vector event_sources;
};

void thorium_actor_profiler_init(struct thorium_actor_profiler *self);
void thorium_actor_profiler_destroy(struct thorium_actor_profiler *self);
void thorium_actor_profiler_profile(struct thorium_actor_profiler *self, int event,
                struct thorium_message *message);

void thorium_actor_profiler_write(struct thorium_actor_profiler *self, const char *script,
                int name, struct core_buffered_file_writer *writer);

#endif /* THORIUM_ACTOR_PROFILER_H */
