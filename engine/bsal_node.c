
#include "bsal_node.h"
#include "bsal_actor.h"

#include <stdlib.h>
#include <stdio.h>

void bsal_node_spawn_actor(struct bsal_node *node, struct bsal_actor *actor)
{

    struct bsal_actor *copy;
    int name;

    if (node->actors == NULL) {
            node->actors = (struct bsal_actor*)malloc(10 * sizeof(struct bsal_actor));
    }

    /* do a copy of the actor wrapper */
    copy = node->actors + node->actor_count;
    *copy = *actor;

    name = bsal_node_assign_name(node);

    bsal_actor_set_name(copy,  name);

    bsal_actor_print(copy);

    node->actor_count++;
}

int bsal_node_assign_name(struct bsal_node *node)
{
    return node->rank + node->ranks * node->actor_count;
}

void bsal_node_construct(struct bsal_node *node, int rank, int ranks, int threads)
{
    /* printf("bsal_node_construct !\n"); */
    node->rank = rank;
    node->ranks = ranks;
    node->threads = threads;

    node->actors = NULL;

    node->actor_count = 0;
}

void bsal_node_destruct(struct bsal_node *node)
{
    if (node->actors != NULL) {
        free(node->actors);
        node->actor_count = 0;
        node->actors = NULL;
    }
}

void bsal_node_start(struct bsal_node *node)
{
    int i;

    for (i = 0; i < node->actor_count; ++i) {
        struct bsal_actor *actor = node->actors + i;
        int name = bsal_actor_name(actor);
        int source = name;

        struct bsal_message message;
        bsal_message_construct(&message, source, name);
        bsal_node_send(node, name, &message);
        bsal_message_destruct(&message);
    }
}

void bsal_node_send(struct bsal_node *node, int name, struct bsal_message *message)
{
        struct bsal_actor *actor;
        struct bsal_actor *pointer;
        bsal_receive_fn_t receive;
        int index;

        index = bsal_node_get_index(node, name);
        actor = node->actors + index;

        bsal_actor_print(actor);
        pointer = bsal_actor_actor(actor);
        receive = bsal_actor_handler(actor);
        /* printf("bsal_node_send %p %p %p %p\n", (void*)actor, (void*)receive,
                        (void*)pointer, (void*)message); */

        receive(pointer, message);
}

int bsal_node_get_index(struct bsal_node *node, int name)
{
    return (name - node->rank) / node->ranks;
}
