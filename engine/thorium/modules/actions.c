
#include "actions.h"

#include <engine/thorium/actor.h>

#include <core/structures/vector.h>

void thorium_actor_add_action_with_sources(struct thorium_actor *self, int tag,
                thorium_actor_receive_fn_t handler, struct core_vector *sources)
{
    int i;
    int source;
    int size;

    size = core_vector_size(sources);

    for (i = 0; i < size; i++) {

        source = core_vector_at_as_int(sources, i);

        thorium_actor_add_action_with_source(self, tag, handler, source);
    }
}

void thorium_actor_add_action(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler)
{

#ifdef THORIUM_ACTOR_DEBUG_10335
    if (tag == 10335) {
        printf("DEBUG actor %d thorium_actor_register 10335\n",
                        thorium_actor_name(self));
    }
#endif

    thorium_actor_add_action_with_source(self, tag, handler, THORIUM_ACTOR_ANYBODY);
}

void thorium_actor_add_action_with_source(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler,
                int source)
{
    thorium_actor_add_action_with_source_and_condition(self, tag,
                    handler, source, NULL, -1);
}

void thorium_actor_add_action_with_condition(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler, int *actual,
                int expected)
{
    thorium_actor_add_action_with_source_and_condition(self, tag, handler, THORIUM_ACTOR_ANYBODY, actual, expected);
}

void thorium_actor_add_action_with_parent(struct thorium_actor *self, int parent_actor, int parent_message,
                thorium_actor_receive_fn_t handler)
{
    thorium_dispatcher_add_action_with_parent(&self->dispatcher, parent_actor, parent_message, handler);
}
