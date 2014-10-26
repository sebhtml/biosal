
#include "process.h"

#include <core/helpers/statistics.h>
#include <core/system/timer.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

#define EVENT_COUNT 10
#define ACTORS_PER_WORKER 1

void process_init(struct thorium_actor *self);
void process_destroy(struct thorium_actor *self);
void process_receive(struct thorium_actor *self, struct thorium_message *message);

void process_send_ping(struct thorium_actor *self);

struct thorium_script process_script = {
    .identifier = SCRIPT_LATENCY_PROCESS,
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
    core_vector_init(&concrete_self->actors, sizeof(int));
    core_vector_init(&concrete_self->children, sizeof(int));
    core_vector_init(&concrete_self->initial_actors, sizeof(int));

    concrete_self->message_count = 0;
    concrete_self->completed = 0;
}

void process_destroy(struct thorium_actor *self)
{
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    core_vector_destroy(&concrete_self->actors);
    core_vector_destroy(&concrete_self->children);
    core_vector_destroy(&concrete_self->initial_actors);
}

void process_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int action;
    void *buffer;
    int leader;
    int worker_count;
    int source;
    int actors_per_worker;
    int i;
    int new_actor;
    struct process *concrete_self;
    struct core_vector actors;
    int name;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    action = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    source = thorium_message_source(message);
    name = thorium_actor_name(self);

    if (action == ACTION_START) {

        core_vector_unpack(&concrete_self->initial_actors, buffer);

        thorium_actor_send_to_self_empty(self, ACTION_GET_NODE_WORKER_COUNT);

    } else if (action == ACTION_GET_NODE_WORKER_COUNT_REPLY) {

        thorium_message_unpack_int(message, 0, &worker_count);

        printf("worker_count %d\n", worker_count);
        actors_per_worker = ACTORS_PER_WORKER;
        concrete_self->size = worker_count * actors_per_worker;

        i = 0;
        while (i < concrete_self->size) {

            thorium_actor_send_to_self_int(self, ACTION_SPAWN,
                            SCRIPT_LATENCY_PROCESS);

            ++i;
        }
    } else if (action == ACTION_SPAWN_REPLY) {

        thorium_message_unpack_int(message, 0, &new_actor);

        core_vector_push_back_int(&concrete_self->children, new_actor);

        printf("receive %d\n", new_actor);

        if (core_vector_size(&concrete_self->children) == concrete_self->size) {

            /*
             * Send to chief.
             */

            leader = core_vector_at_as_int(&concrete_self->initial_actors, 0);
            thorium_actor_send_vector(self, leader, ACTION_PUSH_DATA,
                            &concrete_self->children);

            printf("send ACTION_PUSH_DATA to %d\n", leader);
        }
    } else if (action == ACTION_ASK_TO_STOP) {

        printf("%d receives ACTION_ASK_TO_STOP\n", thorium_actor_name(self));
        thorium_actor_send_range_empty(self, &concrete_self->children, ACTION_ASK_TO_STOP);
        thorium_actor_send_to_self_empty(self, ACTION_STOP);

    } else if (action == ACTION_PUSH_DATA) {

        core_vector_init(&actors, sizeof(int));
        core_vector_unpack(&actors, buffer);

        core_vector_push_back_vector(&concrete_self->actors, &actors);
        core_vector_destroy(&actors);

        if (core_vector_size(&concrete_self->actors) ==
                        core_vector_size(&concrete_self->initial_actors) * concrete_self->size) {

            printf("OK send ACTION_NOTIFY with %d actors\n",
                            (int)core_vector_size(&concrete_self->actors));

            thorium_actor_send_range_vector(self, &concrete_self->actors, ACTION_NOTIFY,
                            &concrete_self->actors);

        }
    } else if (action == ACTION_NOTIFY) {

        core_vector_unpack(&concrete_self->actors, buffer);

        printf("%d has %d friends\n", thorium_actor_name(self),
                        (int)core_vector_size(&concrete_self->actors));

        concrete_self->leader = source;
        process_send_ping(self);

    } else if (action == ACTION_PING) {

        thorium_actor_send_reply_empty(self, ACTION_PING_REPLY);

    } else if (action == ACTION_PING_REPLY) {

        ++concrete_self->message_count;

        if (concrete_self->message_count == EVENT_COUNT) {

            leader = concrete_self->leader;
            thorium_actor_send_empty(self, leader, ACTION_NOTIFY_REPLY);

            printf("%d sent ACTION_NOTIFY_REPLY to %d\n", thorium_actor_name(self),
                            leader);
        } else {

            process_send_ping(self);
        }

    } else if (action == ACTION_NOTIFY_REPLY) {

        ++concrete_self->completed;

        printf("%d received ACTION_NOTIFY_REPLY from %d, %d/%d\n",
                        name, source, concrete_self->completed,
                        (int)core_vector_size(&concrete_self->actors));

        if (concrete_self->completed == (int)core_vector_size(&concrete_self->actors)) {

            thorium_actor_send_range_empty(self, &concrete_self->initial_actors,
                            ACTION_ASK_TO_STOP);
        }
    }
}

void process_send_ping(struct thorium_actor *self)
{
    int target;
    struct process *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    target = rand() % core_vector_size(&concrete_self->actors);

    target = core_vector_at_as_int(&concrete_self->actors, target);

    printf("%d ping %d\n", thorium_actor_name(self), target);

    thorium_actor_send_empty(self, target, ACTION_PING);
}
