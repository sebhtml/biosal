
#include "unitig_manager.h"

#include "unitig_visitor.h"
#include "unitig_walker.h"

#include <genomics/helpers/command.h>

#include <core/patterns/manager.h>
#include <core/patterns/writer_process.h>

#include <core/system/command.h>
#include <core/system/debugger.h>

#include <string.h>

#define STATE_SPAWN_WRITER  0
#define STATE_VISITORS      1
#define STATE_WALKERS       2

/*
#define DISABLED
*/

void biosal_unitig_manager_init(struct thorium_actor *self);
void biosal_unitig_manager_destroy(struct thorium_actor *self);
void biosal_unitig_manager_receive(struct thorium_actor *self, struct thorium_message *message);

void biosal_unitig_manager_receive_answer_from_visitor(struct thorium_actor *self,
                struct thorium_message *message);

struct thorium_script biosal_unitig_manager_script = {
    .identifier = SCRIPT_UNITIG_MANAGER,
    .name = "biosal_unitig_manager",
    .init = biosal_unitig_manager_init,
    .destroy = biosal_unitig_manager_destroy,
    .receive = biosal_unitig_manager_receive,
    .size = sizeof(struct biosal_unitig_manager),
    .description = "The chief executive for unitig visitors and walkers"
};

void biosal_unitig_manager_init(struct thorium_actor *self)
{
    struct biosal_unitig_manager *concrete_self;

    concrete_self = (struct biosal_unitig_manager *)thorium_actor_concrete_actor(self);

    core_vector_init(&concrete_self->spawners, sizeof(int));
    core_vector_init(&concrete_self->graph_stores, sizeof(int));

    core_vector_init(&concrete_self->visitors, sizeof(int));
    core_vector_init(&concrete_self->walkers, sizeof(int));

    concrete_self->completed = 0;
    concrete_self->manager = THORIUM_ACTOR_NOBODY;

    core_timer_init(&concrete_self->timer);

    concrete_self->state = STATE_VISITORS;

    concrete_self->processed_vertices = 0;
    concrete_self->vertices_with_unitig_flag = 0;

    concrete_self->visitor_count_per_worker = 8;
    /*
            thorium_actor_get_suggested_actor_count(self,
               THORIUM_ADAPTATION_FLAG_SMALL_MESSAGES | THORIUM_ADAPTATION_FLAG_SCOPE_WORKER);
               */

    concrete_self->walker_count_per_worker = 8;
    /*
            thorium_actor_get_suggested_actor_count(self,
               THORIUM_ADAPTATION_FLAG_SMALL_MESSAGES | THORIUM_ADAPTATION_FLAG_SCOPE_WORKER);
               */

    CORE_DEBUGGER_ASSERT(concrete_self->visitor_count_per_worker >= 1);

    thorium_actor_log(self, "visitor_count_per_worker = %d",
                    concrete_self->visitor_count_per_worker);
}

void biosal_unitig_manager_destroy(struct thorium_actor *self)
{
    struct biosal_unitig_manager *concrete_self;

    concrete_self = (struct biosal_unitig_manager *)thorium_actor_concrete_actor(self);

    core_vector_destroy(&concrete_self->spawners);
    core_vector_destroy(&concrete_self->graph_stores);

    core_vector_destroy(&concrete_self->walkers);
    core_vector_destroy(&concrete_self->visitors);

    concrete_self->completed = 0;
    concrete_self->manager = THORIUM_ACTOR_NOBODY;

    core_timer_destroy(&concrete_self->timer);
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
void biosal_unitig_manager_receive(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_unitig_manager *concrete_self;
    int tag;
    void *buffer;
    int spawner;
    int expected;
    int script;
    int actor_count;
    int source;
    struct core_string file_name;
    char *directory;
    int argc;
    char **argv;
    char *path;
    int i;
    int size;
    int stride;
    int visitor;

    tag = thorium_message_action(message);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);

    concrete_self = (struct biosal_unitig_manager *)thorium_actor_concrete_actor(self);

    if (tag == ACTION_START) {

        core_vector_unpack(&concrete_self->spawners, buffer);

        spawner = thorium_actor_get_random_spawner(self, &concrete_self->spawners);

        concrete_self->state = STATE_SPAWN_WRITER;

        thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_WRITER_PROCESS);

    } else if (tag == ACTION_SPAWN_REPLY
                    && concrete_self->state == STATE_SPAWN_WRITER) {

        thorium_message_unpack_int(message, 0, &concrete_self->writer_process);

        /*
         * open the file now.
         */

        argc = thorium_actor_argc(self);
        argv = thorium_actor_argv(self);
        directory = biosal_command_get_output_directory(argc, argv);
        core_string_init(&file_name, directory);
        core_string_append(&file_name, "/");
        core_string_append(&file_name, "unitigs.fasta");
        path = core_string_get(&file_name);

        thorium_actor_send_buffer(self, concrete_self->writer_process,
                        ACTION_OPEN, strlen(path) + 1, path);

        core_string_destroy(&file_name);

    } else if (tag == ACTION_OPEN_REPLY
                    && source == concrete_self->writer_process) {
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

        actor_count = concrete_self->visitor_count_per_worker;

        if (concrete_self->state == STATE_WALKERS)
            actor_count = concrete_self->walker_count_per_worker;

        thorium_actor_send_reply_int(self, ACTION_MANAGER_SET_ACTORS_PER_WORKER,
                        actor_count);

    } else if (tag == ACTION_MANAGER_SET_ACTORS_PER_WORKER_REPLY) {

        thorium_actor_send_reply_vector(self, ACTION_START,
                        &concrete_self->spawners);

    } else if (tag == ACTION_START_REPLY
                    && concrete_self->state == STATE_VISITORS
                    && core_vector_size(&concrete_self->visitors) == 0) {

        core_vector_unpack(&concrete_self->visitors, buffer);

        thorium_actor_log(self, "DEBUG the system has %d visitors\n",
                        (int)core_vector_size(&concrete_self->visitors));

        thorium_actor_send_to_supervisor_empty(self, ACTION_START_REPLY);

    } else if (tag == ACTION_START_REPLY
                    && concrete_self->state == STATE_WALKERS
                    && core_vector_size(&concrete_self->walkers) == 0) {

        core_vector_unpack(&concrete_self->walkers, buffer);

        thorium_actor_log(self, "DEBUG the system has %d walkers\n",
                        (int)core_vector_size(&concrete_self->walkers));

        core_timer_start(&concrete_self->timer);
        concrete_self->completed = 0;

        thorium_actor_send_range_int(self, &concrete_self->walkers,
                        ACTION_SET_CONSUMER, concrete_self->writer_process);
        thorium_actor_send_range_vector(self, &concrete_self->walkers,
                        ACTION_START, &concrete_self->graph_stores);

    } else if (tag == ACTION_SET_PRODUCERS) {

#ifdef DISABLED
        thorium_actor_send_reply_empty(self, ACTION_SET_PRODUCERS_REPLY);
        return;
#endif

        core_vector_unpack(&concrete_self->graph_stores, buffer);

        core_timer_start(&concrete_self->timer);

        concrete_self->completed = 0;
        thorium_actor_send_range_vector(self, &concrete_self->visitors,
                        ACTION_START, &concrete_self->graph_stores);

        /*
         * Also, enable the verbose mode for one visitor on each node.
         */

        stride = thorium_actor_node_worker_count(self);
        stride *= concrete_self->visitor_count_per_worker;
        size = core_vector_size(&concrete_self->visitors);

        CORE_DEBUGGER_ASSERT(size % stride == 0);

        i = 0;
        while (i < size) {
            visitor = core_vector_at_as_int(&concrete_self->visitors, i);

            thorium_actor_send_int(self, visitor,
                            ACTION_ENABLE_LOG_LEVEL, LOG_LEVEL_DEFAULT);
            i += stride;
        }

    } else if (tag == ACTION_START_REPLY && concrete_self->state == STATE_VISITORS) {

        biosal_unitig_manager_receive_answer_from_visitor(self, message);

    } else if (tag == ACTION_RESET_REPLY) {

        ++concrete_self->completed;
        expected = core_vector_size(&concrete_self->graph_stores);

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
        expected = core_vector_size(&concrete_self->walkers);

        if (concrete_self->completed % concrete_self->walker_count_per_worker == 0
                        || concrete_self->completed == expected) {
            thorium_actor_log(self, "PROGRESS unitig walkers %d/%d\n",
                        concrete_self->completed,
                        expected);
        }

        if (concrete_self->completed == expected) {

            core_timer_stop(&concrete_self->timer);
            core_timer_print_with_description(&concrete_self->timer, "Walk for unitigs");

            thorium_actor_send_to_supervisor_empty(self, ACTION_SET_PRODUCERS_REPLY);
        }
    }
}

void biosal_unitig_manager_receive_answer_from_visitor(struct thorium_actor *self,
                struct thorium_message *message)
{
    int expected;
    int processed_vertices;
    int vertices_with_unitig_flag;
    struct biosal_unitig_manager *concrete_self;
    void *buffer;
    int offset;

    buffer = thorium_message_buffer(message);
    offset = 0;

    concrete_self = (struct biosal_unitig_manager *)thorium_actor_concrete_actor(self);

    ++concrete_self->completed;
    offset += thorium_message_unpack_int(message, offset, &processed_vertices);
    offset += thorium_message_unpack_int(message, offset, &vertices_with_unitig_flag);

    concrete_self->processed_vertices += processed_vertices;
    concrete_self->vertices_with_unitig_flag += vertices_with_unitig_flag;

    expected = core_vector_size(&concrete_self->visitors);

    if (concrete_self->completed % concrete_self->visitor_count_per_worker == 0
                    || concrete_self->completed == expected) {
        thorium_actor_log(self, "PROGRESS unitig visitors %d/%d\n",
                    concrete_self->completed,
                    expected);
    }

    if (concrete_self->completed == expected) {

        thorium_actor_log(self, "processed_vertices %" PRIu64
                        " vertices_with_unitig_flag %" PRIu64 "",
                        concrete_self->processed_vertices,
                        concrete_self->vertices_with_unitig_flag);

        core_timer_stop(&concrete_self->timer);
        core_timer_print_with_description(&concrete_self->timer, "Visit vertices for unitigs");

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
}
