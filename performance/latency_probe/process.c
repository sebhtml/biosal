
#include "process.h"

#include "target.h"
#include "source.h"

#include <core/helpers/statistics.h>
#include <core/system/timer.h>
#include <core/system/command.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

static void process_init(struct thorium_actor *self);
static void process_destroy(struct thorium_actor *self);
static void process_receive(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script process_script = {
    .identifier = SCRIPT_LATENCY_PROCESS,
    .init = process_init,
    .destroy = process_destroy,
    .receive = process_receive,
    .size = sizeof(struct process),
    .name = "process"
};

static void process_init(struct thorium_actor *self)
{
    struct process *concrete_self;
    int argc;
    char **argv;

    concrete_self = thorium_actor_concrete_actor(self);
    core_vector_init(&concrete_self->actors, sizeof(int));
    core_vector_init(&concrete_self->children, sizeof(int));
    core_vector_init(&concrete_self->target_children, sizeof(int));
    core_vector_init(&concrete_self->initial_actors, sizeof(int));

    concrete_self->actors_per_worker = thorium_actor_get_suggested_actor_count(self,
               THORIUM_ADAPTATION_FLAG_SMALL_MESSAGES | THORIUM_ADAPTATION_FLAG_SCOPE_WORKER);

    concrete_self->completed = 0;

    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);

    concrete_self->event_count = DEFAULT_EVENT_COUNT;

    if (core_command_has_argument(argc, argv, OPTION_EVENT_COUNT)) {
        concrete_self->event_count = core_command_get_argument_value_int(argc, argv, OPTION_EVENT_COUNT);
    }

    core_vector_init(&concrete_self->targets, sizeof(int));

    concrete_self->mode = SCRIPT_SOURCE;

    thorium_actor_add_script(self, SCRIPT_LATENCY_TARGET, &script_target);
    thorium_actor_add_script(self, SCRIPT_SOURCE, &script_source);
}

static void process_destroy(struct thorium_actor *self)
{
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    core_vector_destroy(&concrete_self->actors);
    core_vector_destroy(&concrete_self->children);
    core_vector_destroy(&concrete_self->target_children);
    core_vector_destroy(&concrete_self->initial_actors);

    core_vector_destroy(&concrete_self->targets);
}

static void process_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int action;
    uint64_t total;
    void *buffer;
    int leader;
    int worker_count;
    int source;
    int actors_per_worker;
    int i;
    int new_actor;
    uint64_t elapsed_time;
    double rate;
    double actor_throughput;
    double worker_throughput;
    double elapsed_seconds;
    struct process *concrete_self;
    struct core_vector actors;
    int name;
    int count;
    int nodes;
    int number_of_actors;
    int workers;
    int expected;
    int workers_per_node;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    action = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    source = thorium_message_source(message);
    name = thorium_actor_name(self);
    count = thorium_message_count(message);
    worker_count = thorium_actor_node_worker_count(self);

    if (action == ACTION_START) {

        /*
         * Initial actors can not be NULL.
         */
        CORE_DEBUGGER_ASSERT_NOT_NULL(buffer);

        core_vector_unpack(&concrete_self->initial_actors, buffer);

        thorium_actor_send_to_self_empty(self, ACTION_GET_NODE_WORKER_COUNT);

    } else if (action == ACTION_GET_NODE_WORKER_COUNT_REPLY) {

        thorium_message_unpack_int(message, 0, &worker_count);

        /*
        printf("worker_count %d\n", worker_count);
        */
        actors_per_worker = concrete_self->actors_per_worker;
        concrete_self->size = worker_count * actors_per_worker;

        thorium_actor_send_to_self_int(self, ACTION_SPAWN,
                            SCRIPT_SOURCE);

    } else if (action == ACTION_SPAWN_REPLY
                    && concrete_self->mode == (int)SCRIPT_SOURCE) {

        thorium_message_unpack_int(message, 0, &new_actor);

        core_vector_push_back_int(&concrete_self->children, new_actor);

        /*
        printf("receive %d\n", new_actor);
        */

        if (core_vector_size(&concrete_self->children) == concrete_self->size) {

            /*
             * Send to chief.
             */

            leader = core_vector_at_as_int(&concrete_self->initial_actors, 0);
            thorium_actor_send_vector(self, leader, ACTION_PUSH_DATA,
                            &concrete_self->children);

            printf("%d sends ACTION_PUSH_DATA (%d actors) to leader %d\n", name,
                            concrete_self->size, leader);
        } else {

            thorium_actor_send_to_self_int(self, ACTION_SPAWN,
                            SCRIPT_SOURCE);
        }

    } else if (action == ACTION_SPAWN_REPLY && concrete_self->mode == (int)SCRIPT_LATENCY_TARGET) {

        thorium_message_unpack_int(message, 0, &new_actor);

        core_vector_push_back_int(&concrete_self->target_children, new_actor);

        if (core_vector_size(&concrete_self->target_children) == worker_count) {

            leader = core_vector_at_as_int(&concrete_self->initial_actors, 0);
            thorium_actor_send_vector(self, leader, ACTION_PUSH_DATA,
                            &concrete_self->target_children);
        }

    } else if (action == ACTION_ASK_TO_STOP) {

        leader = core_vector_at_as_int(&concrete_self->initial_actors, 0);

        if (name == leader) {

            core_timer_stop(&concrete_self->timer);
            elapsed_time = core_timer_get_elapsed_nanoseconds(&concrete_self->timer);

            total = core_vector_size(&concrete_self->actors);
            total *= concrete_self->event_count;

            /*
             * Count ACTION_PING and ACTION_PING_REPLY
             */
            total *= 2;
            elapsed_seconds = ((elapsed_time + 0.0) / ( 1000 * 1000 * 1000));

            nodes = thorium_actor_get_node_count(self);
            workers_per_node = thorium_actor_node_worker_count(self);
            workers = nodes * workers_per_node;
            actors_per_worker = concrete_self->actors_per_worker;
            number_of_actors = workers * actors_per_worker;

            rate = (total + 0.0) / elapsed_seconds;
            worker_throughput = rate / workers;
            actor_throughput = rate / number_of_actors;

            printf("PERFORMANCE_COUNTER type = ping-pong\n");
            printf("PERFORMANCE_COUNTER ping-action = ACTION_PING\n");
            printf("PERFORMANCE_COUNTER pong-action = ACTION_PING_REPLY\n");
            printf("PERFORMANCE_COUNTER node-count = %d\n", nodes);
            printf("PERFORMANCE_COUNTER worker-count-per-node = %d\n", workers_per_node);
            printf("PERFORMANCE_COUNTER actor-count-per-worker = %d\n", actors_per_worker);
            printf("PERFORMANCE_COUNTER worker-count = %d\n", workers);
            printf("PERFORMANCE_COUNTER actor-count = %d\n", number_of_actors);
            printf("PERFORMANCE_COUNTER ping-message-count-per-actor = %d\n", concrete_self->event_count);
            printf("PERFORMANCE_COUNTER ping-message-count = %" PRIu64 "\n",
                            total / 2);
            printf("PERFORMANCE_COUNTER pong-message-count = %" PRIu64 "\n",
                            total / 2);
            printf("PERFORMANCE_COUNTER message-count = %" PRIu64 "\n",
                            total);
            printf("PERFORMANCE_COUNTER elapsed-time = %f s\n", elapsed_seconds);

            printf("PERFORMANCE_COUNTER computation-throughput = %f messages / s\n", rate);
            printf("PERFORMANCE_COUNTER node-throughput = %f messages / s\n", rate / nodes);
            printf("PERFORMANCE_COUNTER worker-throughput = %f messages / s\n", worker_throughput);
            printf("PERFORMANCE_COUNTER worker-latency = %d ns\n",
                            (int)((1000 * 1000 * 1000) / worker_throughput));
            printf("PERFORMANCE_COUNTER actor-throughput = %f messages / s\n", actor_throughput);
            printf("PERFORMANCE_COUNTER actor-latency = %d ns\n",
                            (int)((1000 * 1000 * 1000) / actor_throughput));
        }

        printf("%d receives ACTION_ASK_TO_STOP\n", thorium_actor_name(self));
        thorium_actor_send_range_empty(self, &concrete_self->children, ACTION_ASK_TO_STOP);
        thorium_actor_send_range_empty(self, &concrete_self->target_children, ACTION_ASK_TO_STOP);
        thorium_actor_send_to_self_empty(self, ACTION_STOP);

    } else if (action == ACTION_PUSH_DATA
                    && core_vector_size(&concrete_self->actors) !=
                                            core_vector_size(&concrete_self->initial_actors) * concrete_self->size) {

        core_vector_init(&actors, sizeof(int));
        core_vector_unpack(&actors, buffer);

        core_vector_push_back_vector(&concrete_self->actors, &actors);

        core_vector_destroy(&actors);

        if (core_vector_size(&concrete_self->actors) ==
                        core_vector_size(&concrete_self->initial_actors) * concrete_self->size) {

            thorium_actor_send_range_empty(self, &concrete_self->initial_actors, ACTION_SPAWN_TARGETS);
        }
    } else if (action == ACTION_SPAWN_TARGETS) {

        concrete_self->mode = SCRIPT_LATENCY_TARGET;

        for (i = 0; i < worker_count; ++i) {
            thorium_actor_send_to_self_int(self, ACTION_SPAWN, SCRIPT_LATENCY_TARGET);
        }

    } else if (action == ACTION_NOTIFY_REPLY) {

        ++concrete_self->completed;

        /*
         * This will show up also when the whole thing is finished.
         */
        if (concrete_self->completed % concrete_self->actors_per_worker == 0) {
            printf("%d received ACTION_NOTIFY_REPLY from %d, %d/%d\n",
                        name, source, concrete_self->completed,
                        (int)core_vector_size(&concrete_self->actors));
        }

        if (concrete_self->completed == (int)core_vector_size(&concrete_self->actors)) {

            thorium_actor_send_range_empty(self, &concrete_self->initial_actors,
                            ACTION_ASK_TO_STOP);
        }
    } else if (action == ACTION_PUSH_DATA) {

        core_vector_init(&actors, sizeof(int));
        core_vector_unpack(&actors, buffer);

        core_vector_push_back_vector(&concrete_self->targets, &actors);

        core_vector_destroy(&actors);
        expected = core_vector_size(&concrete_self->initial_actors) * worker_count;

        if (core_vector_size(&concrete_self->targets) == expected) {

            printf("%d has %d targets ready\n", name,
                            (int)core_vector_size(&concrete_self->targets));

            printf("%d sends ACTION_NOTIFY (%d targets) to %d actors\n",
                            name,
                            (int)core_vector_size(&concrete_self->targets),
                            (int)core_vector_size(&concrete_self->actors));

            thorium_actor_send_range_vector(self, &concrete_self->actors, ACTION_NOTIFY,
                            &concrete_self->targets);

            printf("Please wait, this can take a while...\n");

            core_timer_init(&concrete_self->timer);
            core_timer_start(&concrete_self->timer);
        }
    }
}

