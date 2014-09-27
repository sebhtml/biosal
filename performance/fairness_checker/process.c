
#include "process.h"

#include <core/helpers/statistics.h>
#include <core/system/timer.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

#define EVENT_COUNT 100000

struct thorium_script process_script = {
    .identifier = SCRIPT_FAIRNESS_PROCESS,
    .init = process_init,
    .destroy = process_destroy,
    .receive = process_receive,
    .size = sizeof(struct process),
    .name = "process"
};

void process_init(struct thorium_actor *self)
{
    struct process *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);
    bsal_vector_init(&concrete_self->times, sizeof(uint64_t));
    bsal_vector_init(&concrete_self->actors, sizeof(int));

    bsal_timer_init(&concrete_self->timer);

    thorium_actor_add_action(self, ACTION_START, process_start);
    thorium_actor_add_action(self, ACTION_ASK_TO_STOP, process_stop);
    thorium_actor_add_action(self, ACTION_PING, process_ping);
    thorium_actor_add_action(self, ACTION_PING_REPLY, process_ping_reply);
    thorium_actor_add_action(self, ACTION_NOTIFY, process_notify);
}

void process_destroy(struct thorium_actor *self)
{
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    bsal_vector_destroy(&concrete_self->times);
    bsal_vector_destroy(&concrete_self->actors);
    bsal_timer_destroy(&concrete_self->timer);
}

void process_receive(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_take_action(self, message);
}

void process_ping(struct thorium_actor *self, struct thorium_message *message)
{
    struct process *concrete_self;
    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    ++concrete_self->received_ping_events;
    thorium_actor_send_reply_empty(self, ACTION_PING_REPLY);
}

void process_start(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    bsal_vector_unpack(&concrete_self->actors, buffer);

    concrete_self->ready = 0;
    bsal_vector_reserve(&concrete_self->times, EVENT_COUNT);

    process_send_ping(self);
}

void process_stop(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_vector intervals;
    uint64_t the_time;
    uint64_t previous_time;
    int i;
    int size;
    struct process *concrete_self;
    int interval;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    bsal_vector_init(&intervals, sizeof(int));

    size = bsal_vector_size(&concrete_self->times);

    for (i = 0; i < size; ++i) {

        the_time = bsal_vector_at_as_uint64_t(&concrete_self->times, i);

#if 0
        printf("%d %" PRIu64 "\n", i, the_time);
#endif

        if (i > 0) {
            interval = the_time - previous_time;

            bsal_vector_push_back(&intervals, &interval);
        }

        previous_time = the_time;
    }

    printf("%s/%d has %d intervals (in nanoseconds) ACTION_PING_REPLY events: %d, ACTION_PING events: %d\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)bsal_vector_size(&intervals), size, concrete_self->received_ping_events);

    bsal_vector_sort_int(&intervals);

    bsal_statistics_print_percentiles_int(&intervals);

    bsal_vector_destroy(&intervals);

    thorium_actor_send_to_self_empty(self, ACTION_STOP);
}

void process_ping_reply(struct thorium_actor *self, struct thorium_message *message)
{
    uint64_t nanoseconds;
    struct process *concrete_self;
    int destination;
    int size;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    nanoseconds = bsal_timer_get_nanoseconds(&concrete_self->timer);

    bsal_vector_push_back(&concrete_self->times, &nanoseconds);

    size = bsal_vector_size(&concrete_self->times);

    if (size % 10000 == 0) {
        printf("%s/%d %d/%d\n", thorium_actor_script_name(self),
                        thorium_actor_name(self), size, EVENT_COUNT);
    }

    if (size < EVENT_COUNT) {
        process_send_ping(self);
    } else {
        destination = bsal_vector_at_as_int(&concrete_self->actors, 0);

        thorium_actor_send_empty(self, destination, ACTION_NOTIFY);
    }
}

void process_send_ping(struct thorium_actor *self)
{
    struct process *concrete_self;
    int destination;
    int size;
    int index;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);

    size = bsal_vector_size(&concrete_self->actors);
    index = rand() % size;

    destination = bsal_vector_at_as_int(&concrete_self->actors, index);

    thorium_actor_send_empty(self, destination, ACTION_PING);
}

void process_notify(struct thorium_actor *self, struct thorium_message *message)
{
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);

    ++concrete_self->ready;

    if (concrete_self->ready == bsal_vector_size(&concrete_self->actors)) {
        thorium_actor_send_range_empty(self, &concrete_self->actors,
                        ACTION_ASK_TO_STOP);
    }
}
