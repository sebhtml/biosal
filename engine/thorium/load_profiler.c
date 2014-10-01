
#include "load_profiler.h"

#include <core/system/debugger.h>
#include <core/system/timer.h>
#include <core/file_storage/output/buffered_file_writer.h>

#include <stdio.h>

void thorium_load_profiler_init(struct thorium_load_profiler *self)
{
    self->profile_begin_count = 0;
    self->profile_end_count = 0;

    core_timer_init(&self->timer);
    core_vector_init(&self->event_start_times, sizeof(uint64_t));
    core_vector_init(&self->event_end_times, sizeof(uint64_t));
    core_vector_init(&self->event_actions, sizeof(int));
}

void thorium_load_profiler_destroy(struct thorium_load_profiler *self)
{
    self->profile_begin_count = 0;
    self->profile_end_count = 0;

    core_vector_destroy(&self->event_start_times);
    core_vector_destroy(&self->event_end_times);
    core_vector_destroy(&self->event_actions);
    core_timer_destroy(&self->timer);
}

void thorium_load_profiler_profile(struct thorium_load_profiler *self, int event, int action)
{
    uint64_t time;

    time = core_timer_get_nanoseconds(&self->timer);

    switch (event) {
        case THORIUM_LOAD_PROFILER_RECEIVE_BEGIN:

            ++self->profile_begin_count;

            core_vector_push_back(&self->event_start_times, &time);
            core_vector_push_back(&self->event_actions, &action);

            break;
        case THORIUM_LOAD_PROFILER_RECEIVE_END:
            CORE_DEBUGGER_ASSERT(self->profile_end_count + 1 == self->profile_begin_count);

            ++self->profile_end_count;
            core_vector_push_back(&self->event_end_times, &time);
            break;
    }
}

void thorium_load_profiler_write(struct thorium_load_profiler *self, const char *script,
                int name, struct core_buffered_file_writer *writer)
{
    int i;
    int size;
    uint64_t start_time;
    uint64_t end_time;
    uint64_t compute_time;
    uint64_t wait_time;
    int action;
    uint64_t last_end_time;

    last_end_time = 0;
    size = core_vector_size(&self->event_start_times);

    for (i = 0; i < size; ++i) {
        start_time = core_vector_at_as_uint64_t(&self->event_start_times, i);
        end_time = core_vector_at_as_uint64_t(&self->event_end_times, i);
        action = core_vector_at_as_int(&self->event_actions, i);

        compute_time = end_time - start_time;
        wait_time = 0;

        if (last_end_time != 0)
            wait_time = start_time - last_end_time;

        /*
         * start_time end_time actor script action compute_time wait_time
         */
        core_buffered_file_writer_printf(writer, "%" PRIu64 "\t%" PRIu64 "\t%d\t%s"
                        "\t0x%x\t%" PRIu64 "\t%" PRIu64 "\n",
                        start_time, end_time, name, script, action,
                        compute_time, wait_time);

        last_end_time = end_time;
    }
}
