
#include "unitig_manager.h"

#include "unitig_visitor.h"
#include "unitig_walker.h"

#include <core/patterns/manager.h>
#include <core/patterns/writer_process.h>

#define UNITIG_VISITOR_COUNT_PER_WORKER     512
#define UNITIG_WALKER_COUNT_PER_WORKER      32

#define STATE_SPAWN_WRITER  0
#define STATE_VISITORS      1
#define STATE_WALKERS       2

struct thorium_script bsal_unitig_manager_script = {
    .identifier = SCRIPT_UNITIG_MANAGER,
    .name = "bsal_unitig_manager",
    .init = bsal_unitig_manager_init,
    .destroy = bsal_unitig_manager_destroy,
    .receive = bsal_unitig_manager_receive,
    .size = sizeof(struct bsal_unitig_manager),
    .description = "The chief executive for unitig visitors and walkers"
};

void bsal_unitig_manager_init(struct thorium_actor *self)
{
    struct bsal_unitig_manager *concrete_self;

    concrete_self = (struct bsal_unitig_manager *)thorium_actor_concrete_actor(self);

    bsal_vector_init(&concrete_self->spawners, sizeof(int));
    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    bsal_vector_init(&concrete_self->visitors, sizeof(int));
    bsal_vector_init(&concrete_self->walkers, sizeof(int));

    concrete_self->completed = 0;
    concrete_self->manager = THORIUM_ACTOR_NOBODY;

    bsal_timer_init(&concrete_self->timer);

    concrete_self->state = STATE_VISITORS;
}

void bsal_unitig_manager_destroy(struct thorium_actor *self)
{
    struct bsal_unitig_manager *concrete_self;

    concrete_self = (struct bsal_unitig_manager *)thorium_actor_concrete_actor(self);

    bsal_vector_destroy(&concrete_self->spawners);
    bsal_vector_destroy(&concrete_self->graph_stores);

    bsal_vector_destroy(&concrete_self->walkers);
    bsal_vector_destroy(&concrete_self->visitors);

    concrete_self->completed = 0;
    concrete_self->manager = THORIUM_ACTOR_NOBODY;

    bsal_timer_destroy(&concrete_self->timer);
}

/*
 * Basically, this actor does this:
 * - spawn visitors
 * - let them visit stuff
 * - kill them.
 * - spawn walkers
 * - let them walk
 * - kill the walkers
 * - return OK
 */
void bsal_unitig_manager_receive(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_unitig_manager *concrete_self;
    int tag;
    void *buffer;
    int spawner;
    int expected;
    int script;
    int actor_count;

    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);

    concrete_self = (struct bsal_unitig_manager *)thorium_actor_concrete_actor(self);

    if (tag == ACTION_START) {

        bsal_vector_unpack(&concrete_self->spawners, buffer);

        spawner = thorium_actor_get_random_spawner(self, &concrete_self->spawners);

        concrete_self->state = STATE_SPAWN_WRITER;

        thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_WRITER_PROCESS);

    } else if (tag == ACTION_SPAWN_REPLY
                    && concrete_self->state == STATE_SPAWN_WRITER) {

        thorium_message_unpack_int(message, 0, &concrete_self->writer_process);

        /*
         * Spawn visitors.
         */
        concrete_self->state = STATE_VISITORS;
        thorium_actor_send_to_self_empty(self, ACTION_PING);

    } else if (tag == ACTION_PING) {
        spawner = thorium_actor_get_random_spawner(self, &concrete_self->spawners);
        thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_MANAGER);

    } else if (tag == ACTION_SPAWN_REPLY) {

        thorium_message_unpack_int(message, 0, &concrete_self->manager);

        script = SCRIPT_UNITIG_VISITOR;

        if (concrete_self->state == STATE_WALKERS) {
            script = SCRIPT_UNITIG_WALKER;
        }
        thorium_actor_send_int(self, concrete_self->manager, ACTION_MANAGER_SET_SCRIPT,
                        script);

    } else if (tag == ACTION_ASK_TO_STOP) {

        thorium_actor_send_empty(self, concrete_self->writer_process,
                        ACTION_ASK_TO_STOP);

        thorium_actor_send_empty(self, concrete_self->manager,
                        ACTION_ASK_TO_STOP);

        thorium_actor_send_to_self_empty(self, ACTION_STOP);

        thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP_REPLY);

    } else if (tag == ACTION_MANAGER_SET_SCRIPT_REPLY) {

        actor_count = UNITIG_VISITOR_COUNT_PER_WORKER;

        if (concrete_self->state == STATE_WALKERS)
            actor_count = UNITIG_WALKER_COUNT_PER_WORKER;

        thorium_actor_send_reply_int(self, ACTION_MANAGER_SET_ACTORS_PER_WORKER,
                        actor_count);

    } else if (tag == ACTION_MANAGER_SET_ACTORS_PER_WORKER_REPLY) {

        thorium_actor_send_reply_vector(self, ACTION_START,
                        &concrete_self->spawners);

    } else if (tag == ACTION_START_REPLY
                    && concrete_self->state == STATE_VISITORS
                    && bsal_vector_size(&concrete_self->visitors) == 0) {

        bsal_vector_unpack(&concrete_self->visitors, buffer);

        printf("DEBUG the system has %d visitors\n",
                        (int)bsal_vector_size(&concrete_self->visitors));

        thorium_actor_send_to_supervisor_empty(self, ACTION_START_REPLY);

    } else if (tag == ACTION_START_REPLY
                    && concrete_self->state == STATE_WALKERS
                    && bsal_vector_size(&concrete_self->walkers) == 0) {

        bsal_vector_unpack(&concrete_self->walkers, buffer);

        printf("DEBUG the system has %d walkers\n",
                        (int)bsal_vector_size(&concrete_self->walkers));

        bsal_timer_start(&concrete_self->timer);
        concrete_self->completed = 0;
        thorium_actor_send_range_vector(self, &concrete_self->walkers,
                        ACTION_START, &concrete_self->graph_stores);

    } else if (tag == ACTION_SET_PRODUCERS) {

        bsal_vector_unpack(&concrete_self->graph_stores, buffer);

        bsal_timer_start(&concrete_self->timer);

        concrete_self->completed = 0;
        thorium_actor_send_range_vector(self, &concrete_self->visitors,
                        ACTION_START, &concrete_self->graph_stores);

    } else if (tag == ACTION_START_REPLY && concrete_self->state == STATE_VISITORS) {

        ++concrete_self->completed;
        expected = bsal_vector_size(&concrete_self->visitors);

        if (concrete_self->completed % UNITIG_VISITOR_COUNT_PER_WORKER == 0
                        || concrete_self->completed == expected) {
            printf("PROGRESS unitig visitors %d/%d\n",
                        concrete_self->completed,
                        expected);
        }

        if (concrete_self->completed == expected) {

            bsal_timer_stop(&concrete_self->timer);
            bsal_timer_print_with_description(&concrete_self->timer, "Visit vertices for unitigs");

            /*
             * Stop the visitor manager and all visitors too.
             */
            thorium_actor_send_empty(self, concrete_self->manager, ACTION_ASK_TO_STOP);

            /*
             * Reset graph stores.
             */
            thorium_actor_send_range_empty(self, &concrete_self->graph_stores,
                            ACTION_RESET);
            concrete_self->completed = 0;
        }

    } else if (tag == ACTION_RESET_REPLY) {

        ++concrete_self->completed;
        expected = bsal_vector_size(&concrete_self->graph_stores);

        if (concrete_self->completed == expected) {
            concrete_self->completed = 0;
            concrete_self->state = STATE_WALKERS;

            /*
             * Go back at the beginning.
             */
            thorium_actor_send_to_self_empty(self, ACTION_PING);
        }
    } else if (tag == ACTION_START_REPLY && concrete_self->state == STATE_WALKERS) {

        ++concrete_self->completed;
        expected = bsal_vector_size(&concrete_self->walkers);

        if (concrete_self->completed % UNITIG_WALKER_COUNT_PER_WORKER == 0
                        || concrete_self->completed == expected) {
            printf("PROGRESS unitig walkers %d/%d\n",
                        concrete_self->completed,
                        expected);
        }

        if (concrete_self->completed == expected) {

            bsal_timer_stop(&concrete_self->timer);
            bsal_timer_print_with_description(&concrete_self->timer, "Walk for unitigs");

            thorium_actor_send_to_supervisor_empty(self, ACTION_SET_PRODUCERS_REPLY);
        }
    }
}

