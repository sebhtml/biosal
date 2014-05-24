
#include "bsal_node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int bsal_node_spawn(struct bsal_node *node, void *actor,
                bsal_actor_receive_fn_t receive)
{
    struct bsal_actor *copy;
    int name;

    /* TODO make sure that we have place above 10 actors */
    if (node->actors == NULL) {
        node->actors = (struct bsal_actor*)malloc(10 * sizeof(struct bsal_actor));
    }

    /* do a copy of the actor wrapper */
    copy = node->actors + node->actor_count;
    bsal_actor_construct(copy, actor, receive);

    name = bsal_node_assign_name(node);

    bsal_actor_set_name(copy, name);
    bsal_actor_set_node(copy, node);

    /*
    printf("bsal_node_spawn new spawn: %i\n",
                    name);
                    */

    /* bsal_actor_print(copy); */

    node->actor_count++;
    node->alive_actors++;

    /* printf("bsal_node_spawn alive %i\n", node->alive_actors); */

    return name;
}

int bsal_node_assign_name(struct bsal_node *node)
{
    return node->rank + node->size * node->actor_count;
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
    node->size = ranks;
    node->threads = threads;

    node->actors = NULL;
    node->actor_count = 0;
    node->dead_actors = 0;
    node->alive_actors = 0;

    /*
    printf("bsal_node_construct Node # %i is online with %i threads"
                    ", the system contains %i nodes (%i threads)\n",
                    node->rank, node->threads, node->size,
                    node->size * node->threads);
                    */

    bsal_fifo_construct(&node->inbound_messages, 16, sizeof(struct bsal_work));
    bsal_fifo_construct(&node->outbound_messages, 16, sizeof(struct bsal_message));
}

void bsal_node_destruct(struct bsal_node *node)
{
    bsal_fifo_destruct(&node->outbound_messages);
    bsal_fifo_destruct(&node->inbound_messages);

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
    int actors;

    /*
    printf("bsal_node_start Node #%i is starting, %i threads,"
                    " %i actors in system\n",
                    node->rank, node->threads, node->actor_count);
                    */

    actors = node->actor_count;

    for (i = 0; i < actors; ++i) {
        struct bsal_actor *actor = node->actors + i;
        int name = bsal_actor_name(actor);
        int source = name;

        struct bsal_message message;
        bsal_message_construct(&message, BSAL_START, source, name, 0, NULL);
        bsal_node_send(node, &message);
        bsal_message_destruct(&message);
    }

    /* wait until all actors are dead... */

    bsal_node_run(node);
}

void bsal_node_run(struct bsal_node *node)
{
    while(1) {

        if (node->alive_actors == 0) {
            break;
        }

        /* pull message */

        struct bsal_work work;
        if (bsal_fifo_pop(&node->inbound_messages, &work)) {

            /* printf("[bsal_node_run] popped message\n"); */
        /* dispatch message */
            bsal_node_work(node, &work);
        }
    }
}

void bsal_node_send(struct bsal_node *node, struct bsal_message *message)
{
        int name;
        int rank;

        name = bsal_message_destination(message);
        rank = bsal_node_actor_rank(node, name);

        if (rank == node->rank) {
            bsal_node_send_here(node, message);
        } else {
            bsal_node_send_elsewhere(node, message);
        }
}

void bsal_node_send_here(struct bsal_node *node, struct bsal_message *message)
{
    bsal_node_receive(node, message);
}

void bsal_node_send_elsewhere(struct bsal_node *node, struct bsal_message *message)
{
    /* TODO to implement */
}

void bsal_node_receive(struct bsal_node *node, struct bsal_message *message)
{
    struct bsal_message *new_message;
    struct bsal_actor *actor;
    int index;
    int rank;
    int name;
    /* int tag; */

    /* tag = bsal_message_tag(message); */

    rank = node->rank;
    name = bsal_message_destination(message);

    /*
    printf("///bsal_node_receive actor %i tag %i\n",
                    name, tag);
                    */

    index = bsal_node_actor_index(node, rank, name);
    actor = node->actors + index;

    /* we need to do a copy of the message */
    /* TODO replace with slab allocator */
    new_message = (struct bsal_message *)malloc(sizeof(struct bsal_message));
    memcpy(new_message, message, sizeof(struct bsal_message));

    struct bsal_work work;
    bsal_work_construct(&work, actor, new_message);
    bsal_fifo_push(&node->inbound_messages, &work);

    /* printf("[bsal_node_receive] pushed work\n"); */
    /* bsal_work_print(&work); */
}

void bsal_node_work(struct bsal_node *node, struct bsal_work *work)
{
    bsal_actor_receive_fn_t receive;
    struct bsal_actor *actor;
    struct bsal_message *message;

    actor = bsal_work_actor(work);
    message = bsal_work_message(work);

    /* bsal_actor_print(actor); */
    receive = bsal_actor_get_receive(actor);
    /* printf("bsal_node_send %p %p %p %p\n", (void*)actor, (void*)receive,
                    (void*)pointer, (void*)message); */

    receive(actor, message);
    int dead = bsal_actor_dead(actor);

    /*
    printf("[bsal_node_work] -> ");
    bsal_work_print(work);
    */

    if (dead) {
        node->alive_actors--;
        node->dead_actors++;

        /* printf("bsal_node_receive alive %i\n", node->alive_actors); */
    }

    /* TODO replace with slab allocator */
    free(message);
}

int bsal_node_actor_index(struct bsal_node *node, int rank, int name)
{
    return (name - rank) / node->size;
}

int bsal_node_actor_rank(struct bsal_node *node, int name)
{
    return name % node->size;
}

int bsal_node_rank(struct bsal_node *node)
{
    return node->rank;
}

int bsal_node_size(struct bsal_node *node)
{
    return node->size;
}
