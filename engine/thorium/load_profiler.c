
#include "load_profiler.h"

#include <core/system/debugger.h>
#include <core/system/timer.h>
#include <core/file_storage/output/buffered_file_writer.h>

#include <stdio.h>

void thorium_load_profiler_init(struct thorium_load_profiler *self)
{
    self->profile_begin_count = 0;
    self->profile_end_count = 0;

    bsal_timer_init(&self->timer);
    bsal_vector_init(&self->event_times, sizeof(uint64_t));
}

void thorium_load_profiler_destroy(struct thorium_load_profiler *self)
{
    self->profile_begin_count = 0;
    self->profile_end_count = 0;

    bsal_vector_destroy(&self->event_times);
    bsal_timer_destroy(&self->timer);
}

void thorium_load_profiler_profile(struct thorium_load_profiler *self, int event)
{
    uint64_t time;

    time = bsal_timer_get_nanoseconds(&self->timer);

    switch (event) {
        case THORIUM_LOAD_PROFILER_RECEIVE_BEGIN:

            ++self->profile_begin_count;

            bsal_vector_push_back(&self->event_times, &time);
            break;
        case THORIUM_LOAD_PROFILER_RECEIVE_END:
            BSAL_DEBUGGER_ASSERT(self->profile_end_count + 1 == self->profile_begin_count);

            ++self->profile_end_count;
            bsal_vector_push_back(&self->event_times, &time);
            break;
    }
}

void thorium_load_profiler_write(struct thorium_load_profiler *self, const char *script,
                int name, struct bsal_buffered_file_writer *writer)
{
    int i;
    int size;
    uint64_t begin_time;
    uint64_t end_time;

    size = bsal_vector_size(&self->event_times);

    BSAL_DEBUGGER_ASSERT(size % 2 == 0);

    for (i = 0; i < size; i += 2) {
        begin_time = bsal_vector_at_as_uint64_t(&self->event_times, i);
        end_time = bsal_vector_at_as_uint64_t(&self->event_times, i + 1);

        bsal_buffered_file_writer_printf(writer, "%" PRIu64 "\t%" PRIu64 "\t%d\t%s\n",
                        begin_time, end_time, name, script);
    }
}
