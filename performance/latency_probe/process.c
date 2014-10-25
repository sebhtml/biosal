
#include "process.h"

#include <core/helpers/statistics.h>
#include <core/system/timer.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

#define EVENT_COUNT 100000

void process_init(struct thorium_actor *self);
void process_destroy(struct thorium_actor *self);
void process_receive(struct thorium_actor *self, struct thorium_message *message);

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
    int worker_count;
    int actors_per_worker;
    int i;
    int new_actor;
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    action = thorium_message_action(message);
    buffer = thorium_message_buffer(message);

    if  (action == ACTION_START) {

        core_vector_unpack(&concrete_self->initial_actors, buffer);

        thorium_actor_send_to_self_empty(self, ACTION_GET_NODE_WORKER_COUNT);

    } else if (action == ACTION_GET_NODE_WORKER_COUNT_REPLY) {

        thorium_message_unpack_int(message, 0, &worker_count);

        printf("worker_count %d\n", worker_count);
        actors_per_worker = 100;
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

            thorium_actor_send_range_empty(self, &concrete_self->children, ACTION_ASK_TO_STOP);
            thorium_actor_send_to_self_empty(self, ACTION_STOP);
        }
    } else if (action == ACTION_ASK_TO_STOP) {

        thorium_actor_send_to_self_empty(self, ACTION_STOP);
    }
}

