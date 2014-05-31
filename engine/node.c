
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*#define BSAL_NODE_DEBUG*/

/*
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_dup.html
 * \see http://www.dartmouth.edu/~rc/classes/intro_mpi/hello_world_ex.html
 * \see https://github.com/GeneAssembly/kiki/blob/master/ki.c#L960
 * \see http://mpi.deino.net/mpi_functions/MPI_Comm_create.html
 */
void bsal_node_init(struct bsal_node *node, int threads,  int *argc,  char ***argv)
{
    int node_name;
    int nodes;
    int i;
    int required;
    int provided;
    int workers;

    node->threads = threads;

    node->argc = *argc;
    node->argv = *argv;

    for (i = 0; i < *argc; i++) {
        if (strcmp((*argv)[i], "-threads-per-node") == 0 && i + 1 < *argc) {
            /*printf("bsal_node_init threads: %s\n",
                            (*argv)[i + 1]);*/
            node->threads = atoi((*argv)[i + 1]);
        }
    }

    if (node->threads < 1) {
        node->threads = 1;
    }

    node->worker_in_main_thread = 1;

    if (node->threads >= 2) {
        node->worker_in_main_thread = 0;
    }

    node->workers_in_threads = 0;
    /* with 2 threads, one of them runs a worker */
    if (node->threads >= 2) {
        node->workers_in_threads = 1;
    }

	/*
     * 3 cases with T threads using MPI_Init_thread:
     *
     * Case 0: T is 1, ask for MPI_THREAD_SINGLE
     * Design: receive, run, and send in main thread
     *
     * Case 1: if T is 2, ask for MPI_THREAD_FUNNELED
     * Design: receive and send in main thread, workers in (T-1) thread
     *
     * Case 2: if T is 3 or more, ask for MPI_THREAD_MULTIPLE
     *
     * Design: if MPI_THREAD_MULTIPLE is provided, receive in main thread, send in 1 thread,
     * workers in (T - 2) threads, otherwise delegate the case to Case 1
     */
    required = MPI_THREAD_SINGLE;
    workers = 1;

    if (node->threads == 1) {
        required = MPI_THREAD_SINGLE;
        workers = node->threads - 0;

    } else if (node->threads == 2) {
        required = MPI_THREAD_FUNNELED;
        workers = node->threads - 1;

    } else if (node->threads >= 3) {
        required = MPI_THREAD_MULTIPLE;

        /* the number of workers depends on whether or not
         * MPI_THREAD_MULTIPLE is provided
         */
    }

    MPI_Init_thread(argc, argv, required, &provided);

    node->send_in_thread = 0;

    if (node->threads >= 3) {

#ifdef BSAL_NODE_DEBUG
        printf("DEBUG= threads: %i\n", node->threads);
#endif
        if (node->provided == MPI_THREAD_MULTIPLE) {
            node->send_in_thread = 1;
            workers = node->threads - 2;
        } else {

#ifdef BSAL_NODE_DEBUG
            printf("DEBUG= MPI_THREAD_MULTIPLE was not provided...\n");
#endif
            workers = node->threads - 1;
        }
    }

    node->provided = provided;
    node->datatype = MPI_BYTE;

    /* make a new communicator for the library and don't use MPI_COMM_WORLD later */
    MPI_Comm_dup(MPI_COMM_WORLD, &node->comm);
    MPI_Comm_rank(node->comm, &node_name);
    MPI_Comm_size(node->comm, &nodes);

    node->name = node_name;
    node->nodes = nodes;

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG threads: %i workers: %i send_in_thread: %i\n",
                    node->threads, workers, node->send_in_thread);
#endif

    bsal_worker_pool_init(&node->worker_pool, workers, node);

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

void bsal_node_set_supervisor(struct bsal_node *node, int name, int supervisor)
{
    struct bsal_actor *actor;

    actor = bsal_node_get_actor_from_name(node, name);
    bsal_actor_set_supervisor(actor, supervisor);
}

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
    return node->name + node->nodes * node->actor_count;
}

void bsal_node_start(struct bsal_node *node)
{
    int i;
    int actors;
    struct bsal_actor *actor;
    int name;
    int source;
    struct bsal_message message;

    if (node->workers_in_threads) {
#ifdef BSAL_NODE_DEBUG
        printf("DEBUG starting %i worker threads\n",
                        bsal_worker_pool_workers(&node->worker_pool));
#endif
        bsal_worker_pool_start(&node->worker_pool);
    }

    if (node->send_in_thread) {
#ifdef BSAL_NODE_DEBUG
        printf("DEBUG starting send thread\n");
#endif
        bsal_node_start_send_thread(node);
    }

    actors = node->actor_count;

    for (i = 0; i < actors; ++i) {
        actor = node->actors + i;
        name = bsal_actor_name(actor);

        /* initial actors are supervised by themselves... */
        bsal_actor_set_supervisor(actor, name);
        source = name;

        bsal_message_init(&message, BSAL_ACTOR_START, source, name, 0, NULL);
        bsal_node_send(node, &message);
        bsal_message_destroy(&message);
    }

    bsal_node_run(node);

#ifdef BSAL_NODE_DEBUG
    printf("BSAL_NODE_DEBUG after loop in bsal_node_run\n");
#endif

    if (node->workers_in_threads) {
        bsal_worker_pool_stop(&node->worker_pool);
    }

    if (node->send_in_thread) {
        pthread_join(node->thread, NULL);
    }
}

int bsal_node_running(struct bsal_node *node)
{
    /* wait until all actors are dead... */
    if (node->alive_actors == 0) {
        return 0;
    }

    return 1;
}

void bsal_node_run(struct bsal_node *node)
{
    struct bsal_message message;

    while (bsal_node_running(node)) {

        /* pull message from network and assign the message to a thread.
         * this code path will call spin_lock if
         * there is a message received.
         */
        if (bsal_node_receive(node, &message)) {
            bsal_node_create_work(node, &message);
        }

        /* the one worker works here if there is only
         * one thread
         */
        if (node->worker_in_main_thread) {
            bsal_worker_pool_run(&node->worker_pool);
        }

        /* with 3 or more threads, the sending operations are
         * in another thread */
        if (node->send_in_thread) {
            continue;
        }

        bsal_node_send_message(node);
    }

}

/* TODO, this needs MPI_THREAD_MULTIPLE, this has not been tested */
void bsal_node_start_send_thread(struct bsal_node *node)
{
    pthread_create(bsal_node_thread(node), NULL, bsal_node_main,
                    node);
}

void *bsal_node_main(void *pointer)
{
    struct bsal_node *node;

    node = (struct bsal_node*)pointer;

    while (bsal_node_running(node)) {
        bsal_node_send_message(node);
    }

    return NULL;
}

pthread_t *bsal_node_thread(struct bsal_node *node)
{
    return &node->thread;
}

void bsal_node_send_message(struct bsal_node *node)
{
    struct bsal_message message;

    /* check for messages to send from from threads */
    /* this call spin_lock only if there is at least
     * a message in the FIFO
     */
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
    destination = node->name;

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
    int node_name;

    actor = bsal_message_source(message);
    node_name = bsal_node_actor_node(node, actor);
    bsal_message_set_source_node(message, node_name);

    actor = bsal_message_destination(message);
    node_name = bsal_node_actor_node(node, actor);
    bsal_message_set_destination_node(message, node_name);
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
    destination = bsal_message_destination_node(message);
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
    int node_name;

    name = bsal_message_destination(message);
    node_name = bsal_node_actor_node(node, name);
    bsal_node_resolve(node, message);

    if (node_name == node->name) {
        /* dispatch locally */
        bsal_node_create_work(node, message);
    } else {

        /* send messages over the network */
        bsal_node_send_outbound_message(node, message);
    }
}

struct bsal_actor *bsal_node_get_actor_from_name(struct bsal_node *node,
                int name)
{
    struct bsal_actor *actor;
    int index;
    int node_name;

    node_name = node->name;
    index = bsal_node_actor_index(node, node_name, name);
    actor = node->actors + index;

    return actor;
}

void bsal_node_create_work(struct bsal_node *node, struct bsal_message *message)
{
    struct bsal_message *new_message;
    struct bsal_actor *actor;
    struct bsal_work work;
    int name;
    int dead;

#ifdef BSAL_NODE_DEBUG
    int tag;
    int source;
#endif

    name = bsal_message_destination(message);

#ifdef BSAL_NODE_DEBUG
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);

    printf("[DEBUG %s %s %i] actor%i (node%i) : actor%i (node%i)"
                    "(tag %i) %i bytes\n",
                    __FILE__, __func__, __LINE__,
                   source, bsal_message_source_node(message),
                   name, bsal_message_destination_node(message),
                   tag, bsal_message_count(message));
#endif

    actor = bsal_node_get_actor_from_name(node, name);

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

int bsal_node_actor_index(struct bsal_node *node, int node_name, int name)
{
    return (name - node_name) / node->nodes;
}

int bsal_node_actor_node(struct bsal_node *node, int name)
{
    return name % node->nodes;
}

int bsal_node_name(struct bsal_node *node)
{
    return node->name;
}

int bsal_node_nodes(struct bsal_node *node)
{
    return node->nodes;
}

void bsal_node_notify_death(struct bsal_node *node, struct bsal_actor *actor)
{
    bsal_actor_init_fn_t destroy;
    /* int name; */
    /*int index;*/

    /*
    node_name = node->name;
    name = bsal_actor_name(actor);
    */

    destroy = bsal_actor_get_destroy(actor);
    destroy(actor);
    bsal_actor_destroy(actor);

    /*index = bsal_node_actor_index(node, node_name, name); */
    /* node->actors[index] = NULL; */

    pthread_spin_lock(&node->death_lock);
    node->alive_actors--;
    node->dead_actors++;
    pthread_spin_unlock(&node->death_lock);
}

int bsal_node_workers(struct bsal_node *node)
{
    return bsal_worker_pool_workers(&node->worker_pool);
}

int bsal_node_argc(struct bsal_node *node)
{
    return node->argc;
}

char **bsal_node_argv(struct bsal_node *node)
{
    return node->argv;
}

int bsal_node_threads(struct bsal_node *node)
{
    return node->threads;
}
