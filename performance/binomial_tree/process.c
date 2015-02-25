
#include "process.h"

#include <core/helpers/statistics.h>
#include <core/system/timer.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

static void process_init(struct thorium_actor *self);
static void process_destroy(struct thorium_actor *self);
static void process_receive(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script process_script =
{
    .identifier = SCRIPT_BINOMIAL_TREE_PROCESS,
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
    core_vector_init(&concrete_self->initial_actors, sizeof(int));

    concrete_self->completed = 0;
}

static void process_destroy(struct thorium_actor *self)
{
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    core_vector_destroy(&concrete_self->initial_actors);
}

static void process_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int action;
    void *buffer;
    int source;
    struct process *concrete_self;
    int name;
    int leader;

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
    int count = thorium_message_count(message);
#endif

    name = thorium_actor_name(self);
    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    action = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    source = thorium_message_source(message);

    if (action == ACTION_START)
    {
        CORE_DEBUGGER_ASSERT(thorium_message_count(message) != 0);
        CORE_DEBUGGER_ASSERT(buffer != NULL);

        core_vector_unpack(&concrete_self->initial_actors, buffer);
        leader = core_vector_at_as_int(&concrete_self->initial_actors, 0);

        if (name == leader)
        {
            printf("%d sends range ACTION_PING\n", name);
            core_vector_print_int(&concrete_self->initial_actors);
            printf("\n");

            thorium_actor_send_range_empty(self, &concrete_self->initial_actors, ACTION_PING);
        }
    }
    else if (action == ACTION_PING)
    {
#ifdef CORE_DEBUGGER_ASSERT_ENABLED
        if (count != 0)
            thorium_message_print(message);
#endif

        CORE_DEBUGGER_ASSERT(count == 0);

        printf("%d receives ACTION_PING from %d, reply with ACTION_PING_REPLY\n", name, source);

        thorium_actor_send_reply_empty(self, ACTION_PING_REPLY);
    }
    else if (action == ACTION_PING_REPLY)
    {
        CORE_DEBUGGER_ASSERT(count == 0);

        ++concrete_self->completed;

        if (concrete_self->completed == (int)core_vector_size(&concrete_self->initial_actors))
        {
            thorium_actor_send_range_empty(self, &concrete_self->initial_actors, ACTION_ASK_TO_STOP);
        }
    }
    else if (action == ACTION_ASK_TO_STOP)
    {
        CORE_DEBUGGER_ASSERT(count == 0);

        thorium_actor_send_to_self_empty(self, ACTION_STOP);
    }
    else
    {
        printf("Warning, unknown action %x\n", action);

        thorium_message_print(message);

        CORE_DEBUGGER_ASSERT(action != ACTION_INVALID);
    }
}
