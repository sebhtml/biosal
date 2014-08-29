
#include "stop.h"

#include <engine/thorium/actor.h>

#include <stdio.h>

void thorium_actor_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message)
{
    /* only the supervisor or self can
     * call this.
     */

    int source = thorium_message_source(message);
    int name = thorium_actor_name(actor);
    int supervisor = thorium_actor_supervisor(actor);

#ifdef THORIUM_ACTOR_STORE_CHILDREN
    int i;
    int child;
#endif

    if (source != name && source != supervisor) {
        printf("actor/%d: permission denied, will not stop (source: %d, name: %d, supervisor: %d\n",
                        thorium_actor_name(actor), source, name, supervisor);
#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
#endif
        return;
    }

#ifdef THORIUM_ACTOR_STORE_CHILDREN
    for (i = 0; i < thorium_actor_child_count(actor); i++) {

        child = thorium_actor_get_child(actor, i);

#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
        printf("actor/%d tells actor %d to stop\n",
                            thorium_actor_name(actor), child);
#endif

        thorium_actor_send_empty(actor, child, ACTION_ASK_TO_STOP);
    }

#ifdef THORIUM_ACTOR_HELPER_DEBUG_STOP
    printf("DEBUG121212 actor/%d dies\n",
                    thorium_actor_name(actor));

    printf("DEBUG actor/%d send ACTION_STOP to self\n",
                    thorium_actor_name(actor));
#endif

#endif

    /*
     * Stop self only if the message was sent by supervisor or self.
     * This is the default behavior and can be overwritten by
     * the concrete actor
     */
    thorium_actor_send_to_self_empty(actor, ACTION_STOP);

    thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP_REPLY);
}

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
int thorium_actor_get_acquaintance(struct thorium_actor *actor, struct bsal_vector *indices,
                int index)
{
    int index2;

    if (index >= bsal_vector_size(indices)) {
        return THORIUM_ACTOR_NOBODY;
    }

    index2 = bsal_vector_at_as_int(indices, index);

    return thorium_actor_get_acquaintance(actor, index2);
}
#endif

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
void thorium_actor_get_acquaintances(struct thorium_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names)
{
    struct bsal_vector_iterator iterator;
    int index;
    int *bucket;
    int name;

#if 0 /* The caller must call the constructor first.
         */
    bsal_vector_init(names, sizeof(int));
#endif

    bsal_vector_iterator_init(&iterator, indices);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket);

        index = *bucket;
        name = thorium_actor_get_acquaintance(actor, index);

        bsal_vector_push_back(names, &name);
    }

    bsal_vector_iterator_destroy(&iterator);
}

#endif

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
void thorium_actor_add_acquaintances(struct thorium_actor *actor,
                struct bsal_vector *names, struct bsal_vector *indices)
{
    struct bsal_vector_iterator iterator;
    int index;
    int *bucket;
    int name;

    bsal_vector_iterator_init(&iterator, names);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket);

        name = *bucket;
        index = thorium_actor_add_acquaintance(actor, name);

#ifdef THORIUM_ACTOR_HELPER_DEBUG
        printf("DEBUG thorium_actor_add_acquaintances name %d index %d\n",
                        name, index);
#endif

        bsal_vector_push_back(indices, &index);
    }

    bsal_vector_iterator_destroy(&iterator);

#ifdef THORIUM_ACTOR_HELPER_DEBUG
    bsal_vector_print_int(names);
    printf("\n");
    bsal_vector_print_int(indices);
    printf("\n");
#endif
}
#endif

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
int thorium_actor_get_acquaintance_index(struct thorium_actor *actor, struct bsal_vector *indices,
                int name)
{
    int i;
    int index;
    int actor_name;
    int size;

    size = bsal_vector_size(indices);


    for (i = 0; i < size; ++i) {
        index = bsal_vector_at_as_int(indices, i);
        actor_name = thorium_actor_get_acquaintance(actor, index);

        if (actor_name == name) {
            return i;
        }
    }

    return -1;
}
#endif


