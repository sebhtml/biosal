
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*#define BSAL_NODE_DEBUG*/

int bsal_node_spawn(struct bsal_node *node, void *pointer,
                struct bsal_actor_vtable *vtable)
{
    struct bsal_actor *actor;
    int name;
    bsal_actor_init_fn_t init;

    pthread_spin_lock(&node->spawn_lock);

    actor = node->actors + node->actor_count;
    bsal_actor_init(actor, pointer, vtable);
    init = bsal_actor_get_init(actor);
    init(actor);

    name = bsal_node_assign_name(node);

    bsal_actor_set_name(actor, name);

    node->actor_count++;
    node->alive_actors++;

    pthread_spin_unlock(&node->spawn_lock);

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
void bsal_node_init(struct bsal_node *node, int threads,  int *argc,  char ***argv)
{
    int rank;
    int ranks;

    MPI_Init(argc, argv);

    node->datatype = MPI_BYTE;

    /* make a new communicator for the library and don't use MPI_COMM_WORLD later */
    MPI_Comm_dup(MPI_COMM_WORLD, &node->comm);
    MPI_Comm_rank(node->comm, &rank);
    MPI_Comm_size(node->comm, &ranks);

    node->rank = rank;
    node->size = ranks;

    bsal_worker_pool_init(&node->worker_pool, threads, node);

    node->actor_count = 0;
    node->actor_capacity = 16384;
    node->dead_actors = 0;
    node->alive_actors = 0;

    /* TODO make sure that we have place above node->actor_capacity actors */
    node->actors = (struct bsal_actor*)malloc(node->actor_capacity * sizeof(struct bsal_actor));

    pthread_spin_init(&node->death_lock, 0);
    pthread_spin_init(&node->spawn_lock, 0);
}

void bsal_node_destroy(struct bsal_node *node)
{
    pthread_spin_destroy(&node->spawn_lock);
    pthread_spin_destroy(&node->death_lock);

    free(node->actors);
    node->actor_count = 0;
    node->actor_capacity = 0;
    node->actors = NULL;

    MPI_Finalize();
}

void bsal_node_start(struct bsal_node *node)
{
    int i;
    int actors;
    struct bsal_actor *actor;
    int name;
    int source;
    struct bsal_message message;

    bsal_worker_pool_start(&node->worker_pool);

    actors = node->actor_count;

    for (i = 0; i < actors; ++i) {
        actor = node->actors + i;
        name = bsal_actor_name(actor);
        source = name;

        bsal_message_init(&message, BSAL_START, source, name, 0, NULL);
        bsal_node_send(node, &message);
        bsal_message_destroy(&message);
    }

    bsal_node_run(node);
}

void bsal_node_run(struct bsal_node *node)
{
    struct bsal_message message;

    while (1) {

        /* wait until all actors are dead... */
        if (node->alive_actors == 0) {
            break;
        }

        /* pull message from network and assign the message to a thread */
        if (bsal_node_receive(node, &message)) {
            bsal_node_create_work(node, &message);
        }

        bsal_worker_pool_run(&node->worker_pool);

        /* check for messages to send from from threads */
        if (bsal_node_pull(node, &message)) {

#ifdef BSAL_NODE_DEBUG
            printf("bsal_node_run pulled tag %i buffer %p\n",
                            bsal_message_tag(&message),
                            bsal_message_buffer(&message));
#endif

            /* send it locally or over the network */
            bsal_node_send(node, &message);
        }
    }

    bsal_worker_pool_stop(&node->worker_pool);
}

int bsal_node_pull(struct bsal_node *node, struct bsal_message *message)
{
    return bsal_worker_pool_pull(&node->worker_pool, message);
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
    int metadata_size;
    MPI_Status status;

    source = MPI_ANY_SOURCE;
    tag = MPI_ANY_TAG;
    destination = node->rank;

    MPI_Iprobe(source, tag, node->comm, &flag, &status);

    if (!flag) {
        return 0;
    }

    MPI_Get_count(&status, node->datatype, &count);

    /* TODO actually allocate (slab allocator) a buffer with count bytes ! */
    buffer = (char *)malloc(count * sizeof(char));

    source = status.MPI_SOURCE;
    tag = status.MPI_TAG;

    MPI_Recv(buffer, count, node->datatype, source, tag,
                    node->comm, &status);

    metadata_size = bsal_message_metadata_size(message);
    count -= metadata_size;

    /* Initially assign the MPI source rank and MPI destination
     * rank for the actor source and actor destination, respectively.
     * Then, read the metadata and resolve the MPI rank from
     * that. The resolved MPI ranks should be the same in all cases
     */
    bsal_message_init(message, tag, source, destination, count, buffer);
    bsal_message_read_metadata(message);
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
    int metadata_size;
    MPI_Request request;
    int all;

    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);
    metadata_size = bsal_message_metadata_size(message);
    all = count + metadata_size;
    destination = bsal_message_destination_rank(message);
    tag = bsal_message_tag(message);

    MPI_Isend(buffer, all, node->datatype, destination, tag,
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
    bsal_node_resolve(node, message);

    if (rank == node->rank) {
        /* dispatch locally */
        bsal_node_create_work(node, message);
    } else {

        /* send messages over the network */
        bsal_node_send_outbound_message(node, message);
    }
}

void bsal_node_create_work(struct bsal_node *node, struct bsal_message *message)
{
    struct bsal_message *new_message;
    struct bsal_actor *actor;
    struct bsal_work work;
    int index;
    int rank;
    int name;
    int dead;

#ifdef BSAL_NODE_DEBUG
    int tag;
    int source;
#endif

    rank = node->rank;
    name = bsal_message_destination(message);

#ifdef BSAL_NODE_DEBUG
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);

    printf("[DEBUG %s %s %i] actor%i (rank%i) : actor%i (rank%i)"
                    "(tag %i) %i bytes\n",
                    __FILE__, __func__, __LINE__,
                   source, bsal_message_source_rank(message),
                   name, bsal_message_destination_rank(message),
                   tag, bsal_message_count(message));
#endif

    index = bsal_node_actor_index(node, rank, name);
    actor = node->actors + index;

    if (actor == NULL) {
        return;
    }

    dead = bsal_actor_dead(actor);

    if (dead) {
        return;
    }

    /* we need to do a copy of the message */
    /* TODO replace with slab allocator */
    new_message = (struct bsal_message *)malloc(sizeof(struct bsal_message));
    memcpy(new_message, message, sizeof(struct bsal_message));

    bsal_work_init(&work, actor, new_message);

    bsal_worker_pool_schedule_work(&node->worker_pool, &work);
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
    bsal_actor_init_fn_t destroy;
    /*int rank;*/
    /* int name; */
    /*int index;*/

    /*
    rank = node->rank;
    name = bsal_actor_name(actor);
    */

    destroy = bsal_actor_get_destroy(actor);
    destroy(actor);
    bsal_actor_destroy(actor);

    /*index = bsal_node_actor_index(node, rank, name); */
    /* node->actors[index] = NULL; */

    pthread_spin_lock(&node->death_lock);
    node->alive_actors--;
    node->dead_actors++;
    pthread_spin_unlock(&node->death_lock);
}
