
#include "node.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

/* options */
#define BSAL_NODE_REUSE_DEAD_INDICES

/* debugging options */
/*
#define BSAL_NODE_DEBUG

#define BSAL_NODE_DEBUG_LOOP
#define BSAL_NODE_DEBUG_RECEIVE_SYSTEM
*/

/*
#define BSAL_NODE_SIMPLE_INITIAL_ACTOR_NAMES
#define BSAL_NODE_DEBUG_SPAWN
#define BSAL_NODE_DEBUG_SUPERVISOR

#define BSAL_NODE_DEBUG_ACTOR_COUNTERS
*/


/*
 * \see http://www.mpich.org/static/docs/v3.1/www3/MPI_Comm_dup.html
 * \see http://www.dartmouth.edu/~rc/classes/intro_mpi/hello_world_ex.html
 * \see https://github.com/GeneAssembly/kiki/blob/master/ki.c#L960
 * \see http://mpi.deino.net/mpi_functions/MPI_Comm_create.html
 */
void bsal_node_init(struct bsal_node *node, int *argc, char ***argv)
{
    int node_name;
    int nodes;
    int i;
    int required;
    int provided;
    int workers;
    int threads;
    char *required_threads;
    int detected;
    int bytes;
    int actor_capacity;

    srand(time(NULL) * getpid());

    node->debug = 0;

#ifdef BSAL_NODE_DEBUG_LOOP
    node->debug = 1;
#endif

    node->maximum_scripts = 1024;
    node->available_scripts = 0;
    node->print_counters = 0;
    bytes = node->maximum_scripts * sizeof(struct bsal_script *);
    node->scripts = (struct bsal_script **)malloc(bytes);

    /* the rank number is needed to decide on
     * the number of threads
     */

    /* the default is 1 thread */
    threads = 1;

    node->threads = threads;

    node->argc = *argc;
    node->argv = *argv;

    required = MPI_THREAD_MULTIPLE;

    MPI_Init_thread(argc, argv, required, &provided);

    /* make a new communicator for the library and don't use MPI_COMM_WORLD later */
    MPI_Comm_dup(MPI_COMM_WORLD, &node->comm);
    MPI_Comm_rank(node->comm, &node_name);
    MPI_Comm_size(node->comm, &nodes);

    for (i = 0; i < *argc; i++) {
        if (strcmp((*argv)[i], "-print-counters") == 0) {
            node->print_counters = 1;
        }
    }

    for (i = 0; i < *argc; i++) {
        if (strcmp((*argv)[i], "-threads-per-node") == 0 && i + 1 < *argc) {
            /*printf("bsal_node_init threads: %s\n",
                            (*argv)[i + 1]);*/

            required_threads = (*argv)[i + 1];

            /* -threads-per-node all
             *
             * \see http://stackoverflow.com/questions/4586405/get-number-of-cpus-in-linux-using-c
             */
#ifdef _SC_NPROCESSORS_ONLN
            if (strcmp(required_threads, "all") == 0) {
                node->threads = sysconf(_SC_NPROCESSORS_ONLN);
#ifdef BSAL_NODE_DEBUG
                printf("DEBUG using all threads: %d\n", node->threads);
#endif
                continue;
            }
#endif

            /* -threads-per-node 5,6,9
             */
            detected = bsal_node_threads_from_string(node, required_threads, node_name);

            if (detected > 0) {
                node->threads = detected;

#ifdef BSAL_NODE_DEBUG
                printf("DEBUG %s node %d : %d threads\n", required_threads,
                                node_name, detected);
#endif

                continue;
            }

            /* -threads-per-node 99
             */
            node->threads = atoi(required_threads);

#ifdef BSAL_NODE_DEBUG
            printf("DEBUG using %d threads\n", node->threads);
#endif
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
    workers = 1;

    if (node->threads == 1) {
        workers = node->threads - 0;

    } else if (node->threads == 2) {
        workers = node->threads - 1;

    } else if (node->threads >= 3) {

        /* the number of workers depends on whether or not
         * MPI_THREAD_MULTIPLE is provided
         */
    }

    node->send_in_thread = 0;

    if (node->threads >= 3) {

#ifdef BSAL_NODE_DEBUG
        printf("DEBUG= threads: %i\n", node->threads);
#endif
        if (node->provided == MPI_THREAD_MULTIPLE) {
            node->send_in_thread = 1;
            workers = node->threads - 2;

        /* assume MPI_THREAD_FUNNELED
         */
        } else {

#ifdef BSAL_NODE_DEBUG
            printf("DEBUG= MPI_THREAD_MULTIPLE was not provided...\n");
#endif
            workers = node->threads - 1;
        }
    }

    node->provided = provided;
    node->datatype = MPI_BYTE;

    node->name = node_name;
    node->nodes = nodes;

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG threads: %i workers: %i send_in_thread: %i\n",
                    node->threads, workers, node->send_in_thread);
#endif

    bsal_worker_pool_init(&node->worker_pool, workers, node);

    actor_capacity = 1048576;
    node->dead_actors = 0;
    node->alive_actors = 0;

    /* TODO make sure that we have place above node->actor_capacity actors */
    bsal_vector_init(&node->actors, sizeof(struct bsal_actor));

    /* it is necessary to reserve because work units will point
     * to actors so their addresses can not be changed
     */
    bsal_vector_reserve(&node->actors, actor_capacity);

    bsal_hash_table_init(&node->actor_names, actor_capacity, sizeof(int), sizeof(int));

    bsal_vector_init(&node->initial_actors, sizeof(int));
    bsal_vector_resize(&node->initial_actors, bsal_node_nodes(node));
    node->received_initial_actors = 0;
    node->ready = 0;

    bsal_lock_init(&node->spawn_and_death_lock);
    bsal_lock_init(&node->script_lock);

    bsal_fifo_init(&node->active_buffers, sizeof(struct bsal_active_buffer));
    bsal_fifo_init(&node->dead_indices, sizeof(int));

    bsal_counter_init(&node->counter);

    node->started = 0;
}

void bsal_node_destroy(struct bsal_node *node)
{
    struct bsal_active_buffer active_buffer;

    bsal_hash_table_destroy(&node->actor_names);
    bsal_vector_destroy(&node->initial_actors);

    bsal_lock_destroy(&node->spawn_and_death_lock);
    bsal_lock_destroy(&node->script_lock);

    bsal_vector_destroy(&node->actors);

    while (bsal_fifo_pop(&node->active_buffers, &active_buffer)) {
        bsal_active_buffer_destroy(&active_buffer);
    }

    bsal_fifo_destroy(&node->active_buffers);
    bsal_fifo_destroy(&node->dead_indices);

    bsal_counter_destroy(&node->counter);

    MPI_Finalize();
}

int bsal_node_threads_from_string(struct bsal_node *node,
                char *required_threads, int index)
{
    int j;
    int commas = 0;
    int start;
    int value;
    int length;


    start = 0;
    length = strlen(required_threads);

    /* first, count commas to change the index
     */
    for (j = 0; j < length; j++) {
        if (required_threads[j] == ',') {
            commas++;
        }
    }

    index %= (commas + 1);

    commas = 0;

    for (j = 0; j < length; j++) {
        if (required_threads[j] == ',') {
            if (index == commas) {
                required_threads[j] = '\0';
                value = atoi(required_threads + start);
                required_threads[j] = ',';
                return value;
            }
            commas++;
            start = j + 1;
        }
    }

    if (commas == 0) {
        return -1;
    }

    /* the last integer is not followed
     * by a comma
     */
    if (index == commas) {
        value = atoi(required_threads + start);
        return value;
    } else {
        /* recursive call...
         * this never happens because the index is changed at the beginning
         * of the function.
         */
        return bsal_node_threads_from_string(node, required_threads, index - commas -1);
    }

    return -1;
}

void bsal_node_set_supervisor(struct bsal_node *node, int name, int supervisor)
{
    struct bsal_actor *actor;

#ifdef BSAL_NODE_DEBUG_SUPERVISOR
    printf("DEBUG bsal_node_set_supervisor %d %d\n", name, supervisor);
#endif

    actor = bsal_node_get_actor_from_name(node, name);

#ifdef BSAL_NODE_DEBUG_SUPERVISOR
    printf("DEBUG set supervisor %d %d %p\n", name, supervisor, (void *)actor);
#endif

    bsal_actor_set_supervisor(actor, supervisor);
}

int bsal_node_actors(struct bsal_node *node)
{
    return bsal_vector_size(&node->actors);
}

int bsal_node_spawn(struct bsal_node *node, int script)
{
    struct bsal_script *script1;
    int size;
    void *state;
    int name;
    struct bsal_actor *actor;

    /* in the current implementation, there can only be one initial
     * actor on each node
     */
    if (node->started == 0 && bsal_node_actors(node) > 0) {
        return -1;
    }

    script1 = bsal_node_find_script(node, script);

    if (script1 == NULL) {
        return -1;
    }

    size = bsal_script_size(script1);

    state = malloc(size);

    name = bsal_node_spawn_state(node, state, script1);

    /* send the initial actor to the master node
     */
    if (bsal_node_actors(node) == 1) {
        struct bsal_message message;
        bsal_message_init(&message, BSAL_NODE_ADD_INITIAL_ACTOR, sizeof(name), &name);
        bsal_node_send_to_node(node, 0, &message);

        /* initial actors are their own spawners.
         */
        actor = bsal_node_get_actor_from_name(node, name);

        bsal_counter_add(bsal_actor_counter(actor), BSAL_COUNTER_SPAWNED_ACTORS, 1);
    }

    bsal_counter_add(&node->counter, BSAL_COUNTER_SPAWNED_ACTORS, 1);

    return name;
}

int bsal_node_spawn_state(struct bsal_node *node, void *state,
                struct bsal_script *script)
{
    struct bsal_actor *actor;
    int name;
    int *bucket;
    int index;

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG bsal_node_spawn_state\n");
#endif

    bsal_lock_lock(&node->spawn_and_death_lock);

    /* add an actor in the vector of actors
     */
    index = bsal_node_allocate_actor_index(node);
    actor = (struct bsal_actor *)bsal_vector_at(&node->actors, index);

    /* actors have random names to enforce
     * the acquaintance paradigm
     */
    name = bsal_node_generate_name(node);

    bsal_actor_init(actor, state, script, name, node);

    /* register the actor name
     */
    bucket = bsal_hash_table_add(&node->actor_names, &name);
    *bucket = index;

#ifdef BSAL_NODE_DEBUG_SPAWN
    printf("DEBUG added Actor %d, index is %d, bucket: %p\n", name, index,
                    (void *)bucket);
#endif

    node->alive_actors++;

    bsal_lock_unlock(&node->spawn_and_death_lock);

    return name;
}

int bsal_node_allocate_actor_index(struct bsal_node *node)
{
    int index;

#ifdef BSAL_NODE_REUSE_DEAD_INDICES
    if (bsal_fifo_pop(&node->dead_indices, &index)) {

#ifdef BSAL_NODE_DEBUG_SPAWN
        printf("DEBUG node/%d bsal_node_allocate_actor_index using an old index %d, size %d\n",
                        bsal_node_name(node),
                        index, bsal_vector_size(&node->actors));
#endif

        return index;
    }
#endif

    index = bsal_vector_size(&node->actors);
    bsal_vector_resize(&node->actors, bsal_vector_size(&node->actors) + 1);

    return index;
}

int bsal_node_generate_name(struct bsal_node *node)
{
    int minimal_value;
    int maximum_value;
    int name;
    int range;
    struct bsal_actor *actor;
    int node_name;
    int difference;
    int nodes;

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG bsal_node_generate_name\n");
#endif

#ifdef BSAL_NODE_SIMPLE_INITIAL_ACTOR_NAMES
    if (bsal_node_actors(node) == 0) {
        return bsal_node_name(node);
    }
#endif

    node_name = bsal_node_name(node);
    actor = NULL;
    minimal_value = 0;
    name = -1;
    maximum_value = 2000000000;
    range = maximum_value - minimal_value;

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG assigning name\n");
#endif

    nodes = bsal_node_nodes(node);

    while (actor != NULL || name < 0) {
        name = rand() % range + minimal_value;

        difference = node_name - name % node->nodes;
        /* add the difference */
        name += difference;

        /* disallow names between 0 and nodes - 1
         */
        if (name < nodes) {
            continue;
        }

        actor = bsal_node_get_actor_from_name(node, name);

        /*printf("DEBUG trying %d... %p\n", name, (void *)actor);*/

    }

#ifdef BSAL_NODE_DEBUG_SPAWN
    printf("DEBUG node %d assigned name %d\n", node->name, name);
#endif

    /*return node->name + node->nodes * bsal_node_actors(node);*/

    return name;
}

void bsal_node_set_initial_actor(struct bsal_node *node, int node_name, int actor)
{
    int *bucket;

    bucket = bsal_vector_at(&node->initial_actors, node_name);
    *bucket = actor;

#ifdef BSAL_NODE_DEBUG_INITIAL_ACTORS
    printf("DEBUG bsal_node_set_initial_actor node %d actor %d, %d actors\n",
                    node_name, actor, bsal_vector_size(&node->initial_actors));
#endif
}

void bsal_node_run(struct bsal_node *node)
{
    printf("----------------------------------------------\n");
    printf("biosal> node/%d: %d threads, %d workers\n", bsal_node_name(node),
                    bsal_node_thread_count(node),
                    bsal_node_worker_count(node));

    node->started = 1;

    if (node->workers_in_threads) {
#ifdef BSAL_NODE_DEBUG
        printf("DEBUG starting %i worker threads\n",
                        bsal_worker_pool_worker_count(&node->worker_pool));
#endif
        bsal_worker_pool_start(&node->worker_pool);
    }

    if (node->send_in_thread) {
#ifdef BSAL_NODE_DEBUG
        printf("DEBUG starting send thread\n");
#endif
        bsal_node_start_send_thread(node);
    }

    bsal_node_run_loop(node);

#ifdef BSAL_NODE_DEBUG
    printf("BSAL_NODE_DEBUG after loop in bsal_node_run\n");
#endif

    if (node->workers_in_threads) {
        bsal_worker_pool_stop(&node->worker_pool);
    }

    if (node->send_in_thread) {
        pthread_join(node->thread, NULL);
    }

    if (node->print_counters) {
        printf("----------------------------------------------\n");
        printf("biosal> Counters for node/%d\n", bsal_node_name(node));
        bsal_counter_print(&node->counter);
    }
}

void bsal_node_start_initial_actor(struct bsal_node *node)
{
    int actors;
    int bytes;
    void *buffer;
    struct bsal_actor *actor;
    int source;
    int i;
    int name;
    struct bsal_message message;

    actors = bsal_vector_size(&node->actors);

    bytes = bsal_vector_pack_size(&node->initial_actors);

#ifdef BSAL_NODE_DEBUG_INITIAL_ACTORS
    printf("DEBUG packing %d initial actors\n",
                    bsal_vector_size(&node->initial_actors));
#endif

    buffer = malloc(bytes);
    bsal_vector_pack(&node->initial_actors, buffer);

    for (i = 0; i < actors; ++i) {
        actor = (struct bsal_actor *)bsal_vector_at(&node->actors, i);
        name = bsal_actor_name(actor);

        /* initial actors are supervised by themselves... */
        bsal_actor_set_supervisor(actor, name);
        source = name;

        bsal_message_init(&message, BSAL_ACTOR_START, bytes, buffer);
        bsal_message_set_source(&message, source);
        bsal_message_set_destination(&message, name);

        bsal_node_send(node, &message);

        bsal_message_destroy(&message);
    }
}

int bsal_node_running(struct bsal_node *node)
{
    /* wait until all actors are dead... */
    if (node->alive_actors > 0) {
        return 1;
    }

    if (bsal_worker_pool_has_messages(&node->worker_pool)) {
        return 1;
    }

    return 0;
}

void bsal_node_test_requests(struct bsal_node *node)
{
    struct bsal_active_buffer active_buffer;
    void *buffer;

    if (bsal_fifo_pop(&node->active_buffers, &active_buffer)) {

        if (bsal_active_buffer_test(&active_buffer)) {
            buffer = bsal_active_buffer_buffer(&active_buffer);

            /* TODO use an allocator
             */
            free(buffer);

            bsal_active_buffer_destroy(&active_buffer);

        /* Just put it back in the FIFO for later */
        } else {
            bsal_fifo_push(&node->active_buffers, &active_buffer);
        }
    }
}

void bsal_node_run_loop(struct bsal_node *node)
{
    struct bsal_message message;

    while (bsal_node_running(node)) {

#ifdef BSAL_NODE_DEBUG_LOOP1
        if (node->debug) {
            printf("DEBUG node/%d is running\n",
                            bsal_node_name(node));
        }
#endif

        /* pull message from network and assign the message to a thread.
         * this code path will call lock if
         * there is a message received.
         */
        if (bsal_node_receive(node, &message)) {
            bsal_node_dispatch_message(node, &message);
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

#ifdef BSAL_NODE_DEBUG_20140601_8
    printf("DEBUG node/%d exited loop\n",
                    bsal_node_name(node));
#endif
}

/* TODO, this needs MPI_THREAD_MULTIPLE, this has not been tested */
void bsal_node_start_send_thread(struct bsal_node *node)
{
    pthread_create(bsal_node_thread(node), NULL, bsal_node_main,
                    node);
}

void *bsal_node_main(void *node1)
{
    struct bsal_node *node;

    node = (struct bsal_node*)node1;

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

    /* Free buffers of active requests
     */
    bsal_node_test_requests(node);

    /* check for messages to send from from threads */
    /* this call lock only if there is at least
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

    /* TODO get return value */
    MPI_Iprobe(source, tag, node->comm, &flag, &status);

    if (!flag) {
        return 0;
    }

    /* TODO get return value */
    MPI_Get_count(&status, node->datatype, &count);

    /* TODO actually allocate (slab allocator) a buffer with count bytes ! */
    buffer = (char *)malloc(count * sizeof(char));

    source = status.MPI_SOURCE;
    tag = status.MPI_TAG;

    /* TODO get return value */
    MPI_Recv(buffer, count, node->datatype, source, tag,
                    node->comm, &status);

    metadata_size = bsal_message_metadata_size(message);
    count -= metadata_size;

    /* Initially assign the MPI source rank and MPI destination
     * rank for the actor source and actor destination, respectively.
     * Then, read the metadata and resolve the MPI rank from
     * that. The resolved MPI ranks should be the same in all cases
     */
    bsal_message_init(message, tag, count, buffer);
    bsal_message_set_source(message, destination);
    bsal_message_set_destination(message, destination);
    bsal_message_read_metadata(message);
    bsal_node_resolve(node, message);

    bsal_counter_add(&node->counter, BSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF, 1);
    bsal_counter_add(&node->counter, BSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF,
                    bsal_message_count(message));

    return 1;
}

int bsal_node_receive_system(struct bsal_node *node, struct bsal_message *message)
{
    int tag;
    void *buffer;
    int name;
    int bytes;
    int i;
    int nodes;
    int source;
    struct bsal_message new_message;

#ifdef BSAL_NODE_DEBUG_RECEIVE_SYSTEM
    int count;

    printf("DEBUG bsal_node_receive_system\n");
#endif

    tag = bsal_message_tag(message);

    if (tag == BSAL_NODE_ADD_INITIAL_ACTOR) {

#ifdef BSAL_NODE_DEBUG_RECEIVE_SYSTEM
        printf("DEBUG BSAL_NODE_ADD_INITIAL_ACTOR received\n");
#endif

        source = bsal_message_source(message);
        buffer = bsal_message_buffer(message);
        name = *(int *)buffer;
        bsal_node_set_initial_actor(node, source, name);

        nodes = bsal_node_nodes(node);
        node->received_initial_actors++;

        if (node->received_initial_actors == nodes) {

#ifdef BSAL_NODE_DEBUG_RECEIVE_SYSTEM
            printf("DEBUG send BSAL_NODE_ADD_INITIAL_ACTOR to all nodes\n");
#endif

            bytes = bsal_vector_pack_size(&node->initial_actors);
            buffer = malloc(bytes);
            bsal_vector_pack(&node->initial_actors, buffer);

            bsal_message_init(&new_message, BSAL_NODE_ADD_INITIAL_ACTORS, bytes, buffer);

            for (i = 0; i < nodes; i++) {
                bsal_node_send_to_node(node, i, &new_message);
            }

            free(buffer);
        }

        return 1;

    } else if (tag == BSAL_NODE_ADD_INITIAL_ACTORS) {

#ifdef BSAL_NODE_DEBUG_RECEIVE_SYSTEM
        printf("DEBUG BSAL_NODE_ADD_INITIAL_ACTORS received\n");
#endif

        buffer = bsal_message_buffer(message);
        source = bsal_message_source_node(message);
        bsal_vector_unpack(&node->initial_actors, buffer);

#ifdef BSAL_NODE_DEBUG_RECEIVE_SYSTEM
        count = bsal_message_count(message);
        printf("DEBUG buffer size: %d, unpacked %d actor names from node/%d\n",
                        count, bsal_vector_size(&node->initial_actors),
                        source);
#endif

        bsal_node_send_to_node_empty(node, source, BSAL_NODE_ADD_INITIAL_ACTORS_REPLY);
        return 1;

    } else if (tag == BSAL_NODE_ADD_INITIAL_ACTORS_REPLY) {

        node->ready++;
        nodes = bsal_node_nodes(node);
        source = bsal_message_source_node(message);

#ifdef BSAL_NODE_DEBUG_RECEIVE_SYSTEM
        printf("node/%d DEBUG BSAL_NODE_ADD_INITIAL_ACTORS_REPLY received from"
                        " %d, %d/%d ready\n", bsal_node_name(node),
                        source, node->ready, nodes);
#endif
        if (node->ready == nodes) {

            for (i = 0; i < nodes; i++) {
                bsal_node_send_to_node_empty(node, i, BSAL_NODE_START);
            }
        }

        return 1;

    } else if (tag == BSAL_NODE_START) {

#ifdef BSAL_NODE_DEBUG_RECEIVE_SYSTEM
        printf("DEBUG node %d starts its initial actor\n",
                        bsal_node_name(node));
#endif

        /* send BSAL_ACTOR_START to initial actor
         * on this node
         */
        bsal_node_start_initial_actor(node);

        return 1;
    }

    return 0;
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

void bsal_node_send_to_node_empty(struct bsal_node *node, int destination, int tag)
{
    struct bsal_message message;
    bsal_message_init(&message, tag, 0, NULL);
    bsal_node_send_to_node(node, destination, &message);
}

void bsal_node_send_to_node(struct bsal_node *node, int destination,
                struct bsal_message *message)
{
    void *new_buffer;
    int count;
    int new_count;
    void *buffer;
    struct bsal_message new_message;
    int tag;

    tag = bsal_message_tag(message);
    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);
    new_count = sizeof(int) * 2 + count;

    /* the runtime system always needs at least
     * 2 int in the buffer for actor names
     * Since we are sending messages between
     * nodes, these names are faked...
     */
    new_buffer = malloc(new_count);
    memcpy(new_buffer, buffer, count);

    /* the metadata size is added by the runtime
     * this is why the value is count and not new_count
     */
    bsal_message_init(&new_message, tag, count, new_buffer);
    bsal_message_set_source(&new_message, bsal_node_name(node));
    bsal_message_set_destination(&new_message, destination);
    bsal_message_write_metadata(&new_message);

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG bsal_node_send_to_node %d\n", destination);
#endif

    bsal_node_send(node, &new_message);
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
    MPI_Request *request;
    int all;
    struct bsal_active_buffer active_buffer;

    buffer = bsal_message_buffer(message);
    count = bsal_message_count(message);
    metadata_size = bsal_message_metadata_size(message);
    all = count + metadata_size;
    destination = bsal_message_destination_node(message);
    tag = bsal_message_tag(message);

    bsal_active_buffer_init(&active_buffer, buffer);
    request = bsal_active_buffer_request(&active_buffer);

    /* TODO get return value */
    MPI_Isend(buffer, all, node->datatype, destination, tag,
                    node->comm, request);

    /* store the MPI_Request to test it later to know when
     * the buffer can be reused
     */
    /* \see http://blogs.cisco.com/performance/mpi_request_free-is-evil/
     */
    /*MPI_Request_free(&request);*/

    bsal_fifo_push(&node->active_buffers, &active_buffer);
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
        bsal_node_dispatch_message(node, message);

#ifdef BSAL_NODE_DEBUG_20140601_8
        if (bsal_message_tag(message) == 1100) {
            printf("DEBUG local message 1100\n");
        }
#endif
        bsal_counter_add(&node->counter, BSAL_COUNTER_SENT_MESSAGES_TO_SELF, 1);
        bsal_counter_add(&node->counter, BSAL_COUNTER_SENT_BYTES_TO_SELF,
                        bsal_message_count(message));

        bsal_counter_add(&node->counter, BSAL_COUNTER_RECEIVED_MESSAGES_FROM_SELF, 1);
        bsal_counter_add(&node->counter, BSAL_COUNTER_RECEIVED_BYTES_FROM_SELF,
                        bsal_message_count(message));

    } else {

        /* send messages over the network */
        bsal_node_send_outbound_message(node, message);

#ifdef BSAL_NODE_DEBUG_20140601_8
        if (bsal_message_tag(message) == 1100) {
            printf("DEBUG outbound message 1100\n");

            node->debug = 1;
        }
#endif

        bsal_counter_add(&node->counter, BSAL_COUNTER_SENT_MESSAGES_NOT_TO_SELF, 1);
        bsal_counter_add(&node->counter, BSAL_COUNTER_SENT_BYTES_NOT_TO_SELF,
                        bsal_message_count(message));
    }
}

struct bsal_actor *bsal_node_get_actor_from_name(struct bsal_node *node,
                int name)
{
    struct bsal_actor *actor;
    int index;

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG bsal_node_get_actor_from_name %d\n", name);
#endif

    if (name < 0) {
        return NULL;
    }

    index = bsal_node_actor_index(node, name);

    if (index < 0) {
        return NULL;
    }

    actor = (struct bsal_actor *)bsal_vector_at(&node->actors, index);

    return actor;
}

void bsal_node_dispatch_message(struct bsal_node *node, struct bsal_message *message)
{
    if (bsal_node_receive_system(node, message)) {
        return;
    }

    /* otherwise, create work and dispatch work to a worker via
     * the worker pool
     */
    bsal_node_create_work(node, message);
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

#ifdef BSAL_NODE_DEBUG_NULL_ACTOR
        printf("DEBUG node/%d: actor/%d does not exist\n", node->name,
                        name);
#endif

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

int bsal_node_actor_index(struct bsal_node *node, int name)
{
    int *bucket;
    int index;

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG calling bsal_hash_table_get with pointer to %d, entries %d\n",
                    name, (int)bsal_hash_table_size(&node->actor_names));
#endif

    bucket = bsal_hash_table_get(&node->actor_names, &name);

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG bsal_node_actor_index %d %p\n", name, (void *)bucket);
#endif

    if (bucket == NULL) {
        return -1;
    }

    index = *bucket;
    return index;
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
    void *state;
    int name;

#ifdef BSAL_NODE_REUSE_DEAD_INDICES
    int index;
#endif

    /* int name; */
    /*int index;*/

    /*
    node_name = node->name;
    name = bsal_actor_name(actor);
    */

    name = bsal_actor_name(actor);

#ifdef BSAL_NODE_REUSE_DEAD_INDICES
    index = bsal_node_actor_index(node, name);

#ifdef BSAL_NODE_DEBUG_SPAWN
    printf("DEBUG node/%d BSAL_NODE_REUSE_DEAD_INDICES push index %d\n",
                   bsal_node_name(node), index);
#endif

#endif

    state = bsal_actor_concrete_actor(actor);

#ifdef BSAL_NODE_DEBUG_ACTOR_COUNTERS
    printf("----------------------------------------------\n");
    printf("Counters for actor/%d\n",
                    name);
    bsal_counter_print(bsal_actor_counter(actor));
    printf("----------------------------------------------\n");
#endif

    /* destroy the abstract actor.
     * this calls destroy on the concrete actor
     * too
     */
    bsal_actor_destroy(actor);

    /* free the bytes of the concrete actor */
    free(state);
    state = NULL;

    /* remove the name from the registry */
    /* maybe a lock is needed for this
     * because spawn also access this attribute
     */

    bsal_lock_lock(&node->spawn_and_death_lock);

    bsal_hash_table_delete(&node->actor_names, &name);

#ifdef BSAL_NODE_REUSE_DEAD_INDICES
    bsal_fifo_push(&node->dead_indices, &index);
#endif

#ifdef BSAL_NODE_DEBUG_20140601_8
    printf("DEBUG bsal_node_notify_death\n");
#endif

    node->alive_actors--;
    node->dead_actors++;
    bsal_lock_unlock(&node->spawn_and_death_lock);

    bsal_counter_add(&node->counter, BSAL_COUNTER_KILLED_ACTORS, 1);

#ifdef BSAL_NODE_DEBUG_20140601_8
    printf("DEBUG exiting bsal_node_notify_death\n");
#endif
}

int bsal_node_worker_count(struct bsal_node *node)
{
    return bsal_worker_pool_worker_count(&node->worker_pool);
}

int bsal_node_argc(struct bsal_node *node)
{
    return node->argc;
}

char **bsal_node_argv(struct bsal_node *node)
{
    return node->argv;
}

int bsal_node_thread_count(struct bsal_node *node)
{
    return node->threads;
}

void bsal_node_add_script(struct bsal_node *node, int name,
                struct bsal_script *script)
{
    int can_add;

    bsal_lock_lock(&node->script_lock);

    can_add = 1;
    if (bsal_node_has_script(node, script)) {
        can_add = 0;
    }

    if (node->available_scripts == node->maximum_scripts) {
        can_add = 0;
    }

    if (can_add) {
        node->scripts[node->available_scripts++] = script;
    }

    bsal_lock_unlock(&node->script_lock);
}

int bsal_node_has_script(struct bsal_node *node, struct bsal_script *script)
{
    if (bsal_node_find_script(node, script->name) != NULL) {
        return 1;
    }
    return 0;
}

struct bsal_script *bsal_node_find_script(struct bsal_node *node, int name)
{
    int i;
    struct bsal_script *script;

    for (i = 0; i < node->available_scripts; i++) {
        script = node->scripts[i];

        if (bsal_script_name(script) == name) {
            return script;
        }
    }

    return NULL;
}

