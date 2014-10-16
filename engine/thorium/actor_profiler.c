
#include "actor_profiler.h"

#include <core/system/debugger.h>
#include <core/system/timer.h>
#include <core/file_storage/output/buffered_file_writer.h>

#include <stdio.h>

void thorium_actor_profiler_init(struct thorium_actor_profiler *self)
{
    self->profile_begin_count = 0;
    self->profile_end_count = 0;

    core_timer_init(&self->timer);
    core_vector_init(&self->event_start_times, sizeof(uint64_t));
    core_vector_init(&self->event_end_times, sizeof(uint64_t));
    core_vector_init(&self->event_actions, sizeof(int));
    core_vector_init(&self->event_sources, sizeof(int));
    core_vector_init(&self->event_counts, sizeof(int));
}

void thorium_actor_profiler_destroy(struct thorium_actor_profiler *self)
{
    self->profile_begin_count = 0;
    self->profile_end_count = 0;

    core_vector_destroy(&self->event_start_times);
    core_vector_destroy(&self->event_end_times);
    core_vector_destroy(&self->event_actions);
    core_vector_destroy(&self->event_sources);
    core_vector_destroy(&self->event_counts);

    core_timer_destroy(&self->timer);
}

void thorium_actor_profiler_profile(struct thorium_actor_profiler *self, int event,
                struct thorium_message *message)
{
    uint64_t time;
    int action;
    int count;
    int source;
    uint64_t communication_time;
    uint64_t last_end_time;
    int i;
    uint64_t threshold;

    /*
     * 10 ms
     */
    threshold = 10 * 1000 * 1000;
    action = thorium_message_action(message);
    count = thorium_message_count(message);
    source = thorium_message_source(message);

    time = core_timer_get_nanoseconds(&self->timer);

    switch (event) {
        case THORIUM_ACTOR_PROFILER_RECEIVE_BEGIN:

            ++self->profile_begin_count;

            core_vector_push_back(&self->event_start_times, &time);
            core_vector_push_back(&self->event_actions, &action);
            core_vector_push_back(&self->event_counts, &count);
            core_vector_push_back(&self->event_sources, &source);

            i = core_vector_size(&self->event_end_times) - 1;

            if (i >= 0) {
                last_end_time = core_vector_at_as_uint64_t(&self->event_end_times, i);
                communication_time = time - last_end_time;

#ifdef THORIUM_MESSAGE_ENABLE_TRACEPOINTS
                if (communication_time >= threshold) {
                    thorium_message_print_tracepoints(message);
                }
#endif
            }

            break;
        case THORIUM_ACTOR_PROFILER_RECEIVE_END:
            CORE_DEBUGGER_ASSERT(self->profile_end_count + 1 == self->profile_begin_count);

            ++self->profile_end_count;
            core_vector_push_back(&self->event_end_times, &time);
            break;
    }
}

void thorium_actor_profiler_write(struct thorium_actor_profiler *self, const char *script,
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
    float ratio;
    int count;
    int source;

    last_end_time = 0;
    size = core_vector_size(&self->event_start_times);

    for (i = 0; i < size; ++i) {
        start_time = core_vector_at_as_uint64_t(&self->event_start_times, i);
        end_time = core_vector_at_as_uint64_t(&self->event_end_times, i);
        action = core_vector_at_as_int(&self->event_actions, i);
        count = core_vector_at_as_int(&self->event_counts, i);
        source = core_vector_at_as_int(&self->event_sources, i);

        compute_time = end_time - start_time;
        wait_time = 0;
        ratio = 0;

        if (last_end_time != 0) {
            wait_time = start_time - last_end_time;
            ratio = (0.0 + compute_time) / wait_time;
        }

        /*
         * Order:
    core_buffered_file_writer_printf(&self->load_profile_writer, "start_time\tend_time\tactor\tscript"
                    "\taction\tcount\tsource\tcommunication_time\tcompute_time\tcompute_to_communication_ratio\n");
         */
        /*
         * start_time end_time actor script action communication_time compute_time compute_to_communication_ratio
         */
        core_buffered_file_writer_printf(writer, "%" PRIu64 "\t%" PRIu64 "\t%d\t%s"
                        "\t0x%x\t%d\t%d" /* action count source */
                        "\t%" PRIu64 "\t%" PRIu64 "\t%0.4f\n", /* wait compute ratio */
                        start_time, end_time, name, script, action,
                        count, source,
                        wait_time,
                        compute_time,
                        ratio);

        last_end_time = end_time;
    }
}
