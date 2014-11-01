
#include "process.h"

#include <core/helpers/statistics.h>
#include <core/system/timer.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

#define EVENT_COUNT 40000
#define ACTORS_PER_WORKER 100
#define PERIOD 500
/*
#define EVENT_COUNT 1
#define ACTORS_PER_WORKER 1
#define PERIOD 5
*/

static void process_init(struct thorium_actor *self);
static void process_destroy(struct thorium_actor *self);
static void process_receive(struct thorium_actor *self, struct thorium_message *message);

static void process_send_ping(struct thorium_actor *self);

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

    concrete_self = thorium_actor_concrete_actor(self);
    core_vector_init(&concrete_self->actors, sizeof(int));
    core_vector_init(&concrete_self->children, sizeof(int));
    core_vector_init(&concrete_self->initial_actors, sizeof(int));

    concrete_self->message_count = 0;
    concrete_self->completed = 0;
}

static void process_destroy(struct thorium_actor *self)
{
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    core_vector_destroy(&concrete_self->actors);
    core_vector_destroy(&concrete_self->children);
    core_vector_destroy(&concrete_self->initial_actors);
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
    double elapsed_seconds;
    struct process *concrete_self;
    struct core_vector actors;
    int name;
    int count;
    int nodes;
    int number_of_actors;
    int workers;
    int workers_per_node;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    action = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    source = thorium_message_source(message);
    name = thorium_actor_name(self);
    count = thorium_message_count(message);

    if (action == ACTION_START) {

        /*
         * Initial actors can not be NULL.
         */
        CORE_DEBUGGER_ASSERT_NOT_NULL(buffer);

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

        leader = core_vector_at_as_int(&concrete_self->initial_actors, 0);

        if (name == leader) {

            core_timer_stop(&concrete_self->timer);
            elapsed_time = core_timer_get_elapsed_nanoseconds(&concrete_self->timer);

            total = core_vector_size(&concrete_self->actors);
            total *= EVENT_COUNT;
            elapsed_seconds = ((elapsed_time + 0.0) / ( 1000 * 1000 * 1000));

            nodes = thorium_actor_get_node_count(self);
            workers_per_node = thorium_actor_node_worker_count(self);
            workers = nodes * workers_per_node;
            actors_per_worker = ACTORS_PER_WORKER;
            number_of_actors = workers * actors_per_worker;

            printf("%d nodes, %d worker threads (%d * %d), %d actors (%d * %d)\n",
                            nodes, workers, nodes, workers_per_node,
                            number_of_actors, workers, actors_per_worker);
            printf("Total sent message count: %" PRIu64 " (%d * %d)\n"
                           "Time: %" PRIu64 " ns (%f s)\n",
                            total, number_of_actors,
                           EVENT_COUNT, elapsed_time, elapsed_seconds);
            rate = (total + 0.0) / elapsed_seconds;

            printf("Computation messaging rate: %f messages / s\n", rate);
            printf("Node messaging rate: %f messages / s\n", rate / nodes);
            printf("Worker messaging rate: %f messages / s\n", rate / workers);
            printf("Actor messaging rate: %f messages / s\n", rate / number_of_actors);
        }

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

            core_timer_init(&concrete_self->timer);
            core_timer_start(&concrete_self->timer);

        }
    } else if (action == ACTION_NOTIFY) {

        core_vector_unpack(&concrete_self->actors, buffer);

        printf("%d has %d friends\n", thorium_actor_name(self),
                        (int)core_vector_size(&concrete_self->actors));

        concrete_self->leader = source;
        process_send_ping(self);

    } else if (action == ACTION_PING) {

#ifdef CORE_DEBUGGER_ENABLE_ASSERT
        if (count != 0) {
            printf("Error, count is %d but should be %d.\n",
                            count, 0);

            thorium_message_print(message);
        }
#endif

        CORE_DEBUGGER_ASSERT_IS_EQUAL_INT(count, 0);
        CORE_DEBUGGER_ASSERT_IS_NULL(buffer);

        thorium_actor_send_reply_empty(self, ACTION_PING_REPLY);

    } else if (action == ACTION_PING_REPLY) {

        ++concrete_self->message_count;

        CORE_DEBUGGER_ASSERT_IS_EQUAL_INT(count, 0);
        CORE_DEBUGGER_ASSERT_IS_NULL(buffer);

        if (concrete_self->message_count % PERIOD == 0 || EVENT_COUNT < 500) {
            printf("progress %d %d/%d\n",
                            name, concrete_self->message_count, EVENT_COUNT);
        }

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

static void process_send_ping(struct thorium_actor *self)
{
    int target;
    struct process *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    target = rand() % core_vector_size(&concrete_self->actors);

    target = core_vector_at_as_int(&concrete_self->actors, target);

    /*
    printf("%d sends ACTION_PING to %d\n", thorium_actor_name(self), target);
    */

    thorium_actor_send_empty(self, target, ACTION_PING);
}
