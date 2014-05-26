
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*#define BSAL_NODE_DEBUG */

int bsal_node_spawn(struct bsal_node *node, void *pointer,
                struct bsal_actor_vtable *vtable)
{
    struct bsal_actor *actor;
    int name;
    bsal_actor_init_fn_t init;

    /* TODO make sure that we have place above 10 actors */
    if (node->actors == NULL) {
        node->actors = (struct bsal_actor*)malloc(10 * sizeof(struct bsal_actor));
    }

    actor = node->actors + node->actor_count;
    bsal_actor_init(actor, pointer, vtable);
    init = bsal_actor_get_init(actor);
    init(actor);

    name = bsal_node_assign_name(node);

    bsal_actor_set_name(actor, name);

    node->actor_count++;
    node->alive_actors++;

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
    node->threads = threads;
    node->thread_array = NULL;

    node->actors = NULL;
    node->actor_count = 0;
    node->dead_actors = 0;
    node->alive_actors = 0;

    pthread_spin_init(&node->death_lock, 0);

    bsal_node_create_threads(node);

    /* with only one thread,  the main thread
     * handles everything.
     */
    if (node->threads == 1) {
        node->thread_for_run = 0;
        node->thread_for_message = 0;
        node->thread_for_work = 0;
    } else if (node->threads > 1) {
        node->thread_for_run = 0;
        node->thread_for_message = 1;
        node->thread_for_work = 1;
    } else {
        printf("Error: the number of threads must be at least 1.\n");
        exit(1);
    }
}

void bsal_node_destroy(struct bsal_node *node)
{
    bsal_node_delete_threads(node);

    pthread_spin_destroy(&node->death_lock);

    if (node->actors != NULL) {
        free(node->actors);
        node->actor_count = 0;
        node->actors = NULL;
    }

    MPI_Finalize();
}

void bsal_node_delete_threads(struct bsal_node *node)
{
    int i = 0;

    if (node->threads <= 0) {
        return;
    }

    for (i = 0; i < node->threads; i++) {
        bsal_thread_destroy(node->thread_array + i);
    }

    free(node->thread_array);
    node->thread_array = NULL;
}

void bsal_node_create_threads(struct bsal_node *node)
{
    int bytes;
    int i;

    if (node->threads <= 0) {
        return;
    }

    bytes = node->threads * sizeof(struct bsal_thread);
    node->thread_array = (struct bsal_thread *)malloc(bytes);

    for (i = 0; i < node->threads; i++) {
        bsal_thread_init(node->thread_array + i, i, node);
    }
}

void bsal_node_start(struct bsal_node *node)
{
    int i;
    int actors;
    struct bsal_actor *actor;
    int name;
    int source;
    struct bsal_message message;

    /* start threads
     *
     * we start at 1 because the thread 0 is
     * used by the main thread...
     */
    for (i = 1; i < node->threads; i++) {
        bsal_thread_start(node->thread_array + i);
    }

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
    int i;

    while(1) {

        /* wait until all actors are dead... */
        if (node->alive_actors == 0) {
            break;
        }

        /* pull message from network and assign the message to a thread */
        if (bsal_node_receive(node, &message)) {
            bsal_node_dispatch(node, &message);
        }

        if (node->threads == 1) {
            /* make the thread work (this is the main thread) */
            bsal_thread_run(bsal_node_select_thread(node));
        }

        /* check for messages to send from from threads */
        if (bsal_node_pull(node, &message)) {

            /* send it locally or over the network */
            bsal_node_send(node, &message);
        }
    }

    /*
     * stop threads
     */

    for (i = 1; i < node->threads; i++) {
        bsal_thread_stop(node->thread_array + i);
    }
}

/* TODO select a thread to pull from */
struct bsal_thread *bsal_node_select_thread_for_message(struct bsal_node *node)
{
    struct bsal_thread *thread;
    int iterations;
    int index;

    index = node->thread_for_message;

#ifdef BSAL_NODE_NO_THREADS
    return node->thread_array + index;
#endif

    /* pick up the first thread with messages
     */
    if (node->threads > 1) {
        iterations = node->threads - 1;
        thread = node->thread_array + index;

        while (iterations > 0
                        && bsal_fifo_empty(bsal_thread_messages(thread))) {

            node->thread_for_message = bsal_node_next_thread(node,
                            index);
            thread = node->thread_array + index;
            iterations--;
        }

        node->thread_for_message = bsal_node_next_thread(node,
                        index);
    }

    /* printf("Selected thread %i for message\n", index); */

    return node->thread_array + index;
}

int bsal_node_next_thread(struct bsal_node *node, int index)
{
    if (node->threads == 1) {
        return 0;
    }

    index++;
    index %= node->threads;
    if (index == 0) {
        index = 1;
    }

    return index;
}

int bsal_node_pull(struct bsal_node *node, struct bsal_message *message)
{
    struct bsal_thread *thread;

    thread = bsal_node_select_thread_for_message(node);

    return bsal_thread_pull_message(thread, message);
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

    printf("[DEBUG %s %s %i] actor%i (rank%i) -> actor%i (rank%i) (tag %i)\n",
                    __FILE__, __func__, __LINE__,
                   source, bsal_message_source_rank(message),
                   name, bsal_message_destination_rank(message),
                   tag);
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

    bsal_node_assign_work(node, &work);
}

/* TODO: select the thread */
struct bsal_thread *bsal_node_select_thread_for_work(struct bsal_node *node)
{
    struct bsal_thread *thread;
    int iterations;
    int index;

    index = node->thread_for_work;

#ifdef BSAL_NODE_NO_THREADS
    return node->thread_array + index;
#endif

    /* pick up the first thread with messages
     */

    if (node->threads > 1) {
        iterations = node->threads - 1;
        thread = node->thread_array + index;

        while (iterations > 0
                        && !bsal_fifo_empty(bsal_thread_messages(thread))) {

            node->thread_for_message = bsal_node_next_thread(node,
                            index);
            thread = node->thread_array + index;
            iterations--;
        }

        node->thread_for_work = bsal_node_next_thread(node,
                        index);
    }

    /*printf("Selected thread %i for work\n", index); */

    return node->thread_array + index;
}

struct bsal_thread *bsal_node_select_thread(struct bsal_node *node)
{
    int index;

    index = node->thread_for_run;
    return node->thread_array + index;
}

void bsal_node_assign_work(struct bsal_node *node, struct bsal_work *work)
{
    struct bsal_thread *thread;

    thread = bsal_node_select_thread_for_work(node);
    bsal_thread_push_work(thread, work);
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
