
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

    node->datatype = MPI_BYTE;

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

    bsal_thread_construct(&node->thread, node);
}

void bsal_node_destruct(struct bsal_node *node)
{
    bsal_thread_destruct(&node->thread);

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
    struct bsal_message message;

    while(1) {

        if (node->alive_actors == 0) {
            break;
        }

        /* pull message from network and assign the message to a thread */
        if (bsal_node_receive(node, &message)) {
            bsal_node_dispatch(node, &message);
        }

        /* make the thread work (currently, this is the main thread) */
        bsal_thread_run(&node->thread);

        /* check for messages to send from from threads */
        if (bsal_node_pull(node, &message)) {

            /* send it locally or over the network */
            bsal_node_send(node, &message);
        }
    }
}

int bsal_node_pull(struct bsal_node *node, struct bsal_message *message)
{
    /* TODO select a thread to pull from */
    return bsal_fifo_pop(bsal_thread_outbound_messages(&node->thread), message);
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Iprobe.html */
/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Recv.html */
/* \see http://www.malcolmmclean.site11.com/www/MpiTutorial/MPIStatus.html */
int bsal_node_receive(struct bsal_node *node, struct bsal_message *message)
{
    char *buffer;
    int count;
    int source;
    int destination;
    int tag;
    int flag;
    MPI_Status status;

    /* printf("DEBUG MPI_Iprobe + MPI_Recv\n"); */

    source = MPI_ANY_SOURCE;
    tag = MPI_ANY_TAG;
    destination = node->rank;

    MPI_Iprobe(source, tag, node->comm, &flag, &status);

    if (!flag) {
        return 0;
    }

    /* printf("bsal_node_receive MPI_Iprobe sucess !\n"); */

    MPI_Get_count(&status, node->datatype, &count);
    buffer = NULL; /* TODO actually allocate a buffer with count bytes ! */
    source = status.MPI_SOURCE;
    tag = status.MPI_TAG;

    MPI_Recv(buffer, count, node->datatype, source, tag,
                    node->comm, &status);

    bsal_message_construct(message, tag, source, destination, count, buffer);
    bsal_node_resolve(node, message);

    return 1;
}

void bsal_node_resolve(struct bsal_node *node, struct bsal_message *message)
{
    int actor;
    int rank;

    actor = bsal_message_source(message);
    rank = bsal_node_actor_rank(node, actor);
    bsal_message_set_source_rank(message, rank);

    actor = bsal_message_destination(message);
    rank = bsal_node_actor_rank(node, actor);
    bsal_message_set_destination_rank(message, rank);
}

/* \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Isend.html */
void bsal_node_send_outbound_message(struct bsal_node *node, struct bsal_message *message)
{
    char *buffer;
    int count;
    /* int source; */
    int destination;
    int tag;
    MPI_Request request;

    bsal_node_resolve(node, message);

    buffer = bsal_message_buffer(message);
    count = bsal_message_bytes(message);
    destination = bsal_message_destination_rank(message);
    tag = bsal_message_tag(message);

    /* source = bsal_message_source_rank(message); */
    /* printf("[bsal_node_send_outbound_message] MPI_Isend %i -> %i tag %i\n",
                    source, destination, tag); */

    MPI_Isend(buffer, count, node->datatype, destination, tag,
                    node->comm, &request);

    /* TODO store the MPI_Request to test it later to know when
     * the buffer can be reused
     */
    MPI_Request_free(&request);
}

void bsal_node_send(struct bsal_node *node, struct bsal_message *message)
{
    int name;
    int rank;

    name = bsal_message_destination(message);
    rank = bsal_node_actor_rank(node, name);

    if (rank == node->rank) {
        /* dispatch locally */
        bsal_node_dispatch(node, message);
    } else {

        /* send messages over the network */
        bsal_node_send_outbound_message(node, message);
    }
}

void bsal_node_dispatch(struct bsal_node *node, struct bsal_message *message)
{
    struct bsal_message *new_message;
    struct bsal_actor *actor;
    int index;
    int rank;
    int name;
    int dead;
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
    dead = bsal_actor_dead(actor);

    if (dead) {
        return;
    }

    /* we need to do a copy of the message */
    /* TODO replace with slab allocator */
    new_message = (struct bsal_message *)malloc(sizeof(struct bsal_message));
    memcpy(new_message, message, sizeof(struct bsal_message));

    struct bsal_work work;
    bsal_work_construct(&work, actor, new_message);

    bsal_node_assign_work(node, &work);

    /* printf("[bsal_node_receive] pushed work\n"); */
    /* bsal_work_print(&work); */
}

void bsal_node_assign_work(struct bsal_node *node, struct bsal_work *work)
{
    /* TODO: select the thread */
    bsal_fifo_push(bsal_thread_inbound_messages(&node->thread), work);
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

void bsal_node_notify_death(struct bsal_node *node, struct bsal_actor *actor)
{
    node->alive_actors--;
    node->dead_actors++;
}
