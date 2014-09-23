#ifndef THORIUM_LOAD_PROFILER_H
#define THORIUM_LOAD_PROFILER_H

#define THORIUM_LOAD_PROFILER_RECEIVE_BEGIN   0
#define THORIUM_LOAD_PROFILER_RECEIVE_END     1

#include <core/structures/vector.h>

#include <core/system/timer.h>

#include <inttypes.h>
#include <stdint.h>

struct bsal_buffered_file_writer;

struct thorium_load_profiler {
    uint64_t profile_begin_count;
    uint64_t profile_end_count;

    struct bsal_timer timer;
    struct bsal_vector event_times;
};

void thorium_load_profiler_init(struct thorium_load_profiler *self);
void thorium_load_profiler_destroy(struct thorium_load_profiler *self);
void thorium_load_profiler_profile(struct thorium_load_profiler *self, int event);

void thorium_load_profiler_write(struct thorium_load_profiler *self, const char *script,
                int name, struct bsal_buffered_file_writer *writer);

#endif /* THORIUM_LOAD_PROFILER_H */
