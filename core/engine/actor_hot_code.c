
#include "actor.h"

#include "worker.h"

#include <core/helpers/vector_helper.h>

void *bsal_actor_concrete_actor(struct bsal_actor *actor)
{
    return actor->state;
}

struct bsal_worker *bsal_actor_worker(struct bsal_actor *actor)
{
    return actor->worker;
}

int bsal_actor_dead(struct bsal_actor *actor)
{
    return actor->dead;
}

/* return 0 if successful
 */
int bsal_actor_trylock(struct bsal_actor *actor)
{
    int result;

    result = bsal_lock_trylock(&actor->receive_lock);

    if (result == BSAL_LOCK_SUCCESS) {
        actor->locked = BSAL_LOCK_LOCKED;
        return result;
    }

    return result;
}

struct bsal_vector *bsal_actor_acquaintance_vector(struct bsal_actor *actor)
{
    return &actor->acquaintance_vector;
}

int bsal_actor_get_acquaintance(struct bsal_actor *actor, int index)
{
    if (index < bsal_vector_size(bsal_actor_acquaintance_vector(actor))) {
        return bsal_vector_helper_at_as_int(bsal_actor_acquaintance_vector(actor),
                        index);
    }

    return BSAL_ACTOR_NOBODY;
}

struct bsal_memory_pool *bsal_actor_get_ephemeral_memory(struct bsal_actor *actor)
{
    struct bsal_worker *worker;

    worker = bsal_actor_worker(actor);

    if (worker == NULL) {
        return NULL;
    }

    return bsal_worker_get_ephemeral_memory(worker);
}


