
#include "source.h"

#include "target.h"

#include <core/helpers/statistics.h>
#include <core/system/timer.h>
#include <core/system/command.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

static void source_init(struct thorium_actor *self);
static void source_destroy(struct thorium_actor *self);
static void source_receive(struct thorium_actor *self, struct thorium_message *message);

static void source_send_ping(struct thorium_actor *self);

static int source_is_important(struct thorium_actor *self);

struct thorium_script script_source = {
    .identifier = SCRIPT_SOURCE,
    .init = source_init,
    .destroy = source_destroy,
    .receive = source_receive,
    .size = sizeof(struct source),
    .name = "source"
};

static void source_init(struct thorium_actor *self)
{
    struct source *concrete_self;
    int argc;
    char **argv;

    concrete_self = thorium_actor_concrete_actor(self);

    concrete_self->message_count = 0;

    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);

    concrete_self->event_count = DEFAULT_EVENT_COUNT;

    if (core_command_has_argument(argc, argv, OPTION_EVENT_COUNT)) {
        concrete_self->event_count = core_command_get_argument_value_int(argc, argv, OPTION_EVENT_COUNT);
    }

    core_vector_init(&concrete_self->targets, sizeof(int));
    core_vector_set_memory_pool(&concrete_self->targets,
                    thorium_actor_get_persistent_memory_pool(self));

    concrete_self->target = -1;
}

static void source_destroy(struct thorium_actor *self)
{
    struct source *concrete_self;

    concrete_self = (struct source *)thorium_actor_concrete_actor(self);

    core_vector_destroy(&concrete_self->targets);
}

static void source_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int action;
    void *buffer;
    int leader;
    int source;
    int count;
    struct source *concrete_self;
    int name;

    concrete_self = (struct source *)thorium_actor_concrete_actor(self);
    action = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    source = thorium_message_source(message);
    name = thorium_actor_name(self);
    count = thorium_message_count(message);

    if (action == ACTION_ASK_TO_STOP) {

        thorium_actor_log(self, "sent %d ACTION_PING messages\n",
                        concrete_self->message_count);

        thorium_actor_send_to_self_empty(self, ACTION_STOP);

    } else if (action == ACTION_NOTIFY) {

#ifdef LATENCY_PROBE_USE_MULTIPLEXER
        thorium_actor_send_to_self_empty(self, ACTION_ENABLE_MULTIPLEXER);
#endif

        CORE_DEBUGGER_ASSERT(core_vector_empty(&concrete_self->targets));

        core_vector_unpack(&concrete_self->targets, buffer);

        if (source_is_important(self)) {
            printf("%d (node %d worker %d) has %d targets\n", thorium_actor_name(self),
                            thorium_actor_node_name(self),
                            thorium_actor_worker_name(self),
                        (int)core_vector_size(&concrete_self->targets));
        }

        concrete_self->leader = source;
        source_send_ping(self);

    } else if (action == ACTION_PING_REPLY) {

        CORE_DEBUGGER_ASSERT(count == 0);

        ++concrete_self->message_count;

        CORE_DEBUGGER_ASSERT_IS_EQUAL_INT(count, 0);
        CORE_DEBUGGER_ASSERT_IS_NULL(buffer);

        if (concrete_self->message_count % PERIOD == 0 || concrete_self->event_count < 500) {
            if (source_is_important(self)) {
                printf("progress %d %d/%d\n",
                            name, concrete_self->message_count, concrete_self->event_count);
            }
        }

        if (concrete_self->message_count == concrete_self->event_count) {

            leader = concrete_self->leader;
            thorium_actor_send_empty(self, leader, ACTION_NOTIFY_REPLY);

            if (source_is_important(self))
                printf("%d (ACTION_PING sent: %d)"
                            " sends ACTION_NOTIFY_REPLY to %d\n", thorium_actor_name(self),
                            concrete_self->message_count,
                            leader);
        } else {

            source_send_ping(self);
        }
    }
}

static void source_send_ping(struct thorium_actor *self)
{
    int target;
    struct source *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    CORE_DEBUGGER_ASSERT(!core_vector_empty(&concrete_self->targets));

    if (concrete_self->target == -1)
        concrete_self->target = thorium_actor_get_random_number(self) % core_vector_size(&concrete_self->targets);

    target = concrete_self->target;
    ++concrete_self->target;
    concrete_self->target %= core_vector_size(&concrete_self->targets);

    target = core_vector_at_as_int(&concrete_self->targets, target);

    /*
    printf("%d sends ACTION_PING to %d\n", thorium_actor_name(self), target);
    */

    thorium_actor_send_empty(self, target, ACTION_PING);
}

static int source_is_important(struct thorium_actor *self)
{
#if 0
    struct source *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

#endif

#ifdef VERBOSITY
    return 1;
#else
    return 0;
#endif
}

