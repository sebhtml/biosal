
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

    /* bsal_actor_print(copy); */

    node->actor_count++;
}

int bsal_node_assign_name(struct bsal_node *node)
{
    return node->rank + node->ranks * node->actor_count;
}

/*
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_dup.html
 * \see http://www.dartmouth.edu/~rc/classes/intro_mpi/hello_world_ex.html
 * \see https://github.com/GeneAssembly/kiki/blob/master/ki.c#L960
 * \see http://mpi.deino.net/mpi_functions/MPI_Comm_create.html
 */
void bsal_node_construct(struct bsal_node *node, int threads,  int *argc,  char ***argv)
{
    int rank;
    int ranks;

    MPI_Init(argc, argv);

    /* make a new communicator for the library and don't use MPI_COMM_WORLD later */
    MPI_Comm_dup(MPI_COMM_WORLD, &node->comm);
    MPI_Comm_rank(node->comm, &rank);
    MPI_Comm_size(node->comm, &ranks);

    /* printf("bsal_node_construct !\n"); */
    node->rank = rank;
    node->ranks = ranks;
    node->threads = threads;

    node->actors = NULL;
    node->actor_count = 0;

    printf("bsal_node_construct Node # %i is online with %i threads"
                    ", the system contains %i nodes (%i threads)\n",
                    node->rank, node->threads, node->ranks,
                    node->ranks * node->threads);
}

void bsal_node_destruct(struct bsal_node *node)
{
    if (node->actors != NULL) {
        free(node->actors);
        node->actor_count = 0;
        node->actors = NULL;
    }

    MPI_Finalize();
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
        bsal_actor_receive_fn_t receive;
        int index;

        index = bsal_node_get_index(node, name);
        actor = node->actors + index;

        /* bsal_actor_print(actor); */
        receive = bsal_actor_get_receive(actor);
        /* printf("bsal_node_send %p %p %p %p\n", (void*)actor, (void*)receive,
                        (void*)pointer, (void*)message); */

        receive(actor, message);
}

int bsal_node_get_index(struct bsal_node *node, int name)
{
    return (name - node->rank) / node->ranks;
}
