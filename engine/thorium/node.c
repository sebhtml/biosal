
/* Must be defined before any header is included.
 * Used for setting affinity.
 *
 * \see http://lists.mcs.anl.gov/pipermail/petsc-dev/2012-May/008453.html
 */

#include "node.h"

#include "transport/active_request.h"

#include <core/structures/vector.h>
#include <core/structures/map_iterator.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/set_iterator.h>

#include <core/helpers/vector_helper.h>

#include <core/system/memory.h>
#include <core/system/tracer.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <inttypes.h>

/* options */
/*
#define BSAL_NODE_DEBUG_ACTOR_COUNTERS
*/
#define BSAL_NODE_REUSE_DEAD_INDICES

/* debugging options */
/*
#define BSAL_NODE_DEBUG

#define BSAL_NODE_DEBUG_RECEIVE_SYSTEM
*/
/*
#define BSAL_NODE_DEBUG_RUN "Yes"
*/


/*
#define BSAL_NODE_SIMPLE_INITIAL_ACTOR_NAMES
#define BSAL_NODE_DEBUG_SPAWN

#define BSAL_NODE_DEBUG_ACTOR_COUNTERS
#define BSAL_NODE_DEBUG_SUPERVISOR
#define BSAL_NODE_DEBUG_LOOP

#define BSAL_NODE_DEBUG_SPAWN_KILL
*/

static struct bsal_node *bsal_node_global_self;

void bsal_node_init(struct bsal_node *node, int *argc, char ***argv)
{
    int i;
    int workers;
    int threads;
    char *required_threads;
    int detected;
    int actor_capacity;
    char *argument;
    int processor;

    /*
    printf("Booting node\n");
    */
    /* register signal handlers
     */
    bsal_node_register_signal_handlers(node);


    bsal_set_init(&node->auto_scaling_actors, sizeof(int));

    node->started = 0;
    node->print_load = 0;
    node->print_structure = 0;
    node->debug = 0;

    bsal_node_global_self = node;

    node->start_time = time(NULL);
    node->last_transport_event_time = node->start_time;
    node->last_report_time = 0;
    node->last_auto_scaling = node->start_time;

    srand(node->start_time * getpid());

    /*
     * Build memory pools
     */

    bsal_memory_pool_init(&node->actor_memory_pool, 2097152);
    bsal_memory_pool_disable(&node->actor_memory_pool);
    bsal_memory_pool_init(&node->inbound_message_memory_pool, 2097152);
    bsal_memory_pool_disable(&node->inbound_message_memory_pool);
    bsal_memory_pool_init(&node->node_message_memory_pool, 2097152);
    bsal_memory_pool_disable(&node->node_message_memory_pool);

    bsal_transport_init(&node->transport, node, argc, argv);
    node->provided = bsal_transport_get_provided(&node->transport);
    node->name = bsal_transport_get_rank(&node->transport);
    node->nodes = bsal_transport_get_size(&node->transport);

#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    bsal_ring_queue_init(&node->outbound_buffers, sizeof(struct bsal_active_request));
#endif

#ifdef BSAL_NODE_CHECK_TRANSPORT
    node->use_transport = 1;
#endif

    node->debug = 0;

#ifdef BSAL_NODE_DEBUG_LOOP
    node->debug = 1;
#endif

    node->print_counters = 0;
    bsal_map_init(&node->scripts, sizeof(int), sizeof(struct bsal_script *));

    /* the rank number is needed to decide on
     * the number of threads
     */

    /* the default is 1 thread */
    threads = 1;

    node->threads = threads;
    node->argc = *argc;
    node->argv = *argv;

    for (i = 0; i < node->argc; i++) {
        argument = node->argv[i];
        if (strcmp(argument, "-print-counters") == 0) {
            node->print_counters = 1;
        } else if (strcmp(argument, "-print-load") == 0) {
            node->print_load = 1;
        } else if (strcmp(argument, "-print-structure") == 0) {
            node->print_structure = 1;
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
            detected = bsal_node_threads_from_string(node, required_threads, node->name);

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
     * 3 cases with T threads using transport_init:
     *
     * Case 0: T is 1, ask for BSAL_THREAD_SINGLE
     * Design: receive, run, and send in main thread
     *
     * Case 1: if T is 2, ask for BSAL_THREAD_FUNNELED
     * Design: receive and send in main thread, workers in (T-1) thread
     *
     * Case 2: if T is 3 or more, ask for BSAL_THREAD_MULTIPLE
     *
     * Design: if BSAL_THREAD_MULTIPLE is provided, receive in main thread, send in 1 thread,
     * workers in (T - 2) threads, otherwise delegate the case to Case 1
     */
    workers = 1;

    if (node->threads == 1) {
        workers = node->threads - 0;

    } else if (node->threads == 2) {
        workers = node->threads - 1;

    } else if (node->threads >= 3) {

        /* the number of workers depends on whether or not
         * BSAL_THREAD_MULTIPLE is provided
         */
    }

    node->send_in_thread = 0;

    if (node->threads >= 3) {

#ifdef BSAL_NODE_DEBUG
        printf("DEBUG= threads: %i\n", node->threads);
#endif
        if (node->provided == BSAL_THREAD_MULTIPLE) {
            node->send_in_thread = 1;
            workers = node->threads - 2;

        /* assume BSAL_THREAD_FUNNELED
         */
        } else {

#ifdef BSAL_NODE_DEBUG
            printf("DEBUG= BSAL_THREAD_MULTIPLE was not provided...\n");
#endif
            workers = node->threads - 1;
        }
    }


#ifdef BSAL_NODE_DEBUG
    printf("DEBUG threads: %i workers: %i send_in_thread: %i\n",
                    node->threads, workers, node->send_in_thread);
#endif

    bsal_worker_pool_init(&node->worker_pool, workers, node);

    actor_capacity = 131072;
    node->dead_actors = 0;
    node->alive_actors = 0;

    bsal_vector_init(&node->actors, sizeof(struct bsal_actor));

    /* it is necessary to reserve because work units will point
     * to actors so their addresses can not be changed
     */
    bsal_vector_reserve(&node->actors, actor_capacity);

    /*
     * Multiply by 2 to avoid resizing
     */
    bsal_map_init_with_capacity(&node->actor_names, sizeof(int), sizeof(int), actor_capacity * 2);

    bsal_vector_init(&node->initial_actors, sizeof(int));

    /*printf("BEFORE\n");*/
    bsal_vector_resize(&node->initial_actors, bsal_node_nodes(node));
    /*printf("AFTER\n");*/

    node->received_initial_actors = 0;
    node->ready = 0;

    bsal_lock_init(&node->spawn_and_death_lock);
    bsal_lock_init(&node->script_lock);

    bsal_queue_init(&node->dead_indices, sizeof(int));

    bsal_counter_init(&node->counter);


    processor = workers;

    if (node->nodes != 1) {
        processor = -1;
    }

    bsal_set_affinity(processor);

    printf("%s booted node %d (%d nodes), threads: %d, workers: %d, pacing: %d\n",
                BSAL_NODE_THORIUM_PREFIX,
                    node->name,
            node->nodes,
            node->threads,
            bsal_worker_pool_worker_count(&node->worker_pool),
            1);
}

void bsal_node_destroy(struct bsal_node *node)
{
    struct bsal_active_request active_request;
    void *buffer;

    bsal_set_destroy(&node->auto_scaling_actors);

    bsal_memory_pool_destroy(&node->actor_memory_pool);
    bsal_memory_pool_destroy(&node->inbound_message_memory_pool);
    bsal_memory_pool_destroy(&node->node_message_memory_pool);

    bsal_map_destroy(&node->actor_names);

    /*printf("BEFORE DESTROY\n");*/
    bsal_vector_destroy(&node->initial_actors);
    /*printf("AFTER DESTROY\n");*/

    bsal_map_destroy(&node->scripts);
    bsal_lock_destroy(&node->spawn_and_death_lock);
    bsal_lock_destroy(&node->script_lock);

    bsal_vector_destroy(&node->actors);

    bsal_queue_destroy(&node->dead_indices);

    bsal_worker_pool_destroy(&node->worker_pool);

    while (bsal_transport_dequeue_active_request(&node->transport, &active_request)) {
        buffer = bsal_active_request_buffer(&active_request);

        bsal_memory_free(buffer);
        bsal_active_request_destroy(&active_request);
    }

    bsal_transport_destroy(&node->transport);
    bsal_counter_destroy(&node->counter);

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

    if (name == BSAL_ACTOR_NOBODY) {
        return;
    }

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

    state = bsal_memory_pool_allocate(&node->actor_memory_pool, size);

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

#ifdef BSAL_NODE_DEBUG_SPAWN_KILL
    printf("DEBUG node/%d bsal_node_spawn actor/%d script/%x\n",
                    bsal_node_name(node),
                    name, script);
#endif

    return name;
}

int bsal_node_spawn_state(struct bsal_node *node, void *state,
                struct bsal_script *script)
{
    struct bsal_actor *actor;
    int name;
    int *bucket;
    int index;

    /* can not spawn any more actor
     */
    if (bsal_vector_size(&node->actors) == bsal_vector_capacity(&node->actors)) {
        printf("Error: can not spawn more actors, capacity was reached...\n");
        return BSAL_ACTOR_NOBODY;
    }

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
    bucket = bsal_map_add(&node->actor_names, &name);
    *bucket = index;

    /*
     * Make the name visible to all threads
     */

    bsal_memory_fence();

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
    if (bsal_queue_dequeue(&node->dead_indices, &index)) {

#ifdef BSAL_NODE_DEBUG_SPAWN
        printf("DEBUG node/%d bsal_node_allocate_actor_index using an old index %d, size %d\n",
                        bsal_node_name(node),
                        index, bsal_vector_size(&node->actors));
#endif

        return index;
    }
#endif

    index = (int)bsal_vector_size(&node->actors);
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

    /* reserve  the first numbers
     */
    minimal_value = 4* bsal_node_nodes(node);
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
    float load;

    if (node->print_counters) {
        printf("----------------------------------------------\n");
        printf("biosal> node/%d: %d threads, %d workers\n", bsal_node_name(node),
                    bsal_node_thread_count(node),
                    bsal_node_worker_count(node));
    }

    node->started = 1;

    if (node->workers_in_threads) {
#ifdef BSAL_NODE_DEBUG_RUN
        printf("BSAL_NODE_DEBUG_RUN DEBUG starting %i worker threads\n",
                        bsal_worker_pool_worker_count(&node->worker_pool));
#endif
        bsal_worker_pool_start(&node->worker_pool);
    }


    if (node->send_in_thread) {
#ifdef BSAL_NODE_DEBUG_RUN
        printf("BSAL_NODE_DEBUG_RUN starting send thread\n");
#endif
        bsal_node_start_send_thread(node);
    }

#ifdef BSAL_NODE_DEBUG_RUN
    printf("BSAL_NODE_DEBUG_RUN entering loop in bsal_node_run\n");
#endif

    bsal_node_run_loop(node);

#ifdef BSAL_NODE_DEBUG_RUN
    printf("BSAL_NODE_DEBUG_RUN after loop in bsal_node_run\n");
#endif

    if (node->workers_in_threads) {
        bsal_worker_pool_stop(&node->worker_pool);
    }

    if (node->print_load) {
        bsal_worker_pool_print_load(&node->worker_pool, BSAL_WORKER_POOL_LOAD_EPOCH);
        bsal_worker_pool_print_load(&node->worker_pool, BSAL_WORKER_POOL_LOAD_LOOP);
    }

    if (node->send_in_thread) {
        bsal_thread_join(&node->thread);
    }

    /* Always print counters at the end, this is useful.
     */
    if (node->print_counters) {
        bsal_node_print_counters(node);
    }

    /* Print global load for this node... */

    load = bsal_worker_pool_get_computation_load(&node->worker_pool);
    printf("%s node/%d COMPUTATION_LOAD %.2f\n",
                    BSAL_NODE_THORIUM_PREFIX,
                    bsal_node_name(node),
                    load);
}

void bsal_node_start_initial_actor(struct bsal_node *node)
{
    int actors;
    int bytes;
    void *buffer;
    struct bsal_actor *actor;
    int i;
    int name;
    struct bsal_message message;

    actors = bsal_vector_size(&node->actors);

    bytes = bsal_vector_pack_size(&node->initial_actors);

#ifdef BSAL_NODE_DEBUG_INITIAL_ACTORS
    printf("DEBUG packing %d initial actors\n",
                    bsal_vector_size(&node->initial_actors));
#endif

    buffer = bsal_memory_pool_allocate(&node->inbound_message_memory_pool, bytes);
    bsal_vector_pack(&node->initial_actors, buffer);

    for (i = 0; i < actors; ++i) {
        actor = (struct bsal_actor *)bsal_vector_at(&node->actors, i);
        name = bsal_actor_name(actor);

        /* initial actors are supervised by themselves... */
        bsal_actor_set_supervisor(actor, name);

        bsal_message_init(&message, BSAL_ACTOR_START, bytes, buffer);

        bsal_node_send_to_actor(node, name, &message);

        bsal_message_destroy(&message);
    }

    /*
    bsal_memory_pool_free(&node->inbound_message_memory_pool, buffer);
    */
}

int bsal_node_running(struct bsal_node *node)
{
    time_t current_time;
    int elapsed;

    /* wait until all actors are dead... */
    if (node->alive_actors > 0) {

#ifdef BSAL_NODE_DEBUG_RUN
        printf("BSAL_NODE_DEBUG_RUN alive workers > 0\n");
#endif
        return 1;
    }

    current_time = time(NULL);

    elapsed = current_time - node->last_transport_event_time;

    /*
     * Shut down if there is no actor and
     * if no message were pulled for a duration of at least
     * 4 seconds.
     */
    if (node->use_transport
                    && elapsed < 4) {
#ifdef BSAL_NODE_DEBUG_RUN
        printf("BSAL_NODE_DEBUG_RUN a message was received in the last period %d %d\n",
                        (int)current_time, (int)node->last_transport_event_time);
#endif
        return 1;
    }

    return 0;
}

/* TODO, this needs BSAL_THREAD_MULTIPLE, this has not been tested */
void bsal_node_start_send_thread(struct bsal_node *node)
{
    bsal_thread_init(&node->thread, bsal_node_main, node);
    bsal_thread_start(&node->thread);
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
            buffer = bsal_memory_pool_allocate(&node->node_message_memory_pool, bytes);
            bsal_vector_pack(&node->initial_actors, buffer);

            bsal_message_init(&new_message, BSAL_NODE_ADD_INITIAL_ACTORS, bytes, buffer);

            for (i = 0; i < nodes; i++) {
                bsal_node_send_to_node(node, i, &new_message);
            }

            bsal_memory_pool_free(&node->node_message_memory_pool, buffer);
        }

        return 1;

    } else if (tag == BSAL_NODE_ADD_INITIAL_ACTORS) {

#ifdef BSAL_NODE_DEBUG_RECEIVE_SYSTEM
        printf("DEBUG BSAL_NODE_ADD_INITIAL_ACTORS received\n");
#endif

        buffer = bsal_message_buffer(message);
        source = bsal_message_source_node(message);

        bsal_vector_destroy(&node->initial_actors);
        bsal_vector_init(&node->initial_actors, 0);
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

#ifdef BSAL_NODE_CHECK_TRANSPORT
        /* disable transport layer
         * if there is only one node
         */
        if (node->nodes == 1) {
            node->use_transport = 0;
        }
#endif

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
    size_t new_count;
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
    new_buffer = bsal_memory_pool_allocate(&node->node_message_memory_pool, new_count);
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

int bsal_node_has_actor(struct bsal_node *self, int name)
{
    int node_name;

    node_name = bsal_node_actor_node(self, name);

    if (node_name == self->name) {

            return 1;

        /* The block below is disabled.
         */
#ifdef BSAL_NODE_CHECK_DEAD_ACTOR
        /* maybe the actor is dead already !
         */
        if (bsal_node_get_actor_from_name(self, name) != NULL) {
            return 1;
        }
#endif
    }

    return 0;
}

void bsal_node_send(struct bsal_node *node, struct bsal_message *message)
{
    int name;

#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    int worker_name;
    struct bsal_worker *worker;
    struct bsal_active_request recycled_buffer;
    void *buffer;
#endif

    /* Check the message to see
     * if it is a special message.
     *
     * System message have no buffer to free because they have no buffer.
     */
    if (bsal_node_send_system(node, message)) {
        return;
    }

    name = bsal_message_destination(message);
    bsal_transport_resolve(&node->transport, message);

#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    /* First, verify if this is a recycled message
     */
    if (bsal_message_is_recycled(message)) {

        worker_name = bsal_message_get_worker(message);
        buffer = bsal_message_buffer(message);

        if (worker_name >= 0) {
            worker = bsal_worker_pool_get_worker(&node->worker_pool, worker_name);
            if (!bsal_worker_free_buffer(worker, buffer)) {

                bsal_active_request_init(&recycled_buffer, buffer, worker_name);
                bsal_ring_queue_enqueue(&node->outbound_buffers, &recycled_buffer);
                bsal_active_request_destroy(&recycled_buffer);
            }
        } else {

            /* This is a buffer allocated during the reception
             * from another BIOSAL node
             */

            bsal_memory_pool_free(&node->inbound_message_memory_pool, buffer);
        }
        return;
    }
#endif

    /* If the actor is local, dispatch the message locally
     */
    if (bsal_node_has_actor(node, name)) {

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

    /* Otherwise, the message must be sent to another BIOSAL
     * node
     */
    } else {
        /* If transport layer
         * is disable, this will never be reached anyway
         */
        /* send messages over the network */
        bsal_transport_send(&node->transport, message);

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
    void *buffer;

    if (bsal_node_receive_system(node, message)) {

        /*
         * The buffer must be freed.
         */
        buffer = bsal_message_buffer(message);

        if (buffer != NULL) {
            bsal_memory_pool_free(&node->node_message_memory_pool, buffer);
        }

        return;
    }

    /* otherwise, create work and dispatch work to a worker via
     * the worker pool
     */
    bsal_node_create_work(node, message);
}

void bsal_node_create_work(struct bsal_node *node, struct bsal_message *message)
{
    struct bsal_actor *actor;
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
#ifdef BSAL_NODE_DEBUG_NULL_ACTOR
        printf("DEBUG node/%d: actor/%d is dead\n", node->name,
                        name);
#endif
        return;
    }

#if 0
    printf("DEBUG node enqueue message\n");
#endif
    bsal_worker_pool_enqueue_message(&node->worker_pool, message);
}

int bsal_node_actor_index(struct bsal_node *node, int name)
{
    int *bucket;
    int index;

#ifdef BSAL_NODE_DEBUG
    printf("DEBUG calling bsal_map with pointer to %d, entries %d\n",
                    name, (int)bsal_map_table_size(&node->actor_names));
#endif

    bucket = bsal_map_get(&node->actor_names, &name);

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

    if (node->print_structure) {
        bsal_node_print_structure(node, actor);
    }

    name = bsal_actor_name(actor);

#ifdef BSAL_NODE_DEBUG_SPAWN_KILL
    printf("DEBUG bsal_node_notify_death node/%d actor/%d script/%x\n",
                    bsal_node_name(node),
                    name,
                    bsal_actor_script(actor));
#endif

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
    bsal_memory_pool_free(&node->actor_memory_pool, state);
    state = NULL;

    /* remove the name from the registry */
    /* maybe a lock is needed for this
     * because spawn also access this attribute
     */

    bsal_lock_lock(&node->spawn_and_death_lock);

    bsal_map_delete(&node->actor_names, &name);

    /*
     * Make this change visible
     */
    bsal_memory_fence();

#ifdef BSAL_NODE_REUSE_DEAD_INDICES
    bsal_queue_enqueue(&node->dead_indices, &index);
#endif

#ifdef BSAL_NODE_DEBUG_20140601_8
    printf("DEBUG bsal_node_notify_death\n");
#endif

    /* Remove the actor from the list of auto-scaling
     * actors
     */

    if (bsal_set_find(&node->auto_scaling_actors, &name)) {
        bsal_set_delete(&node->auto_scaling_actors, &name);

        /* This fence is not required.
         */
        /*
        bsal_memory_fence();
        */
    }

    node->alive_actors--;
    node->dead_actors++;

    if (node->alive_actors == 0) {
        printf("%s all local actors are dead now, %d alive actors, %d dead actors\n",
                        BSAL_NODE_THORIUM_PREFIX,
                        node->alive_actors, node->dead_actors);
    }

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

    if (can_add) {
        bsal_map_add_value(&node->scripts, &name, &script);
    }

#ifdef BSAL_NODE_DEBUG_SCRIPT_SYSTEM
    printf("DEBUG added script %x %p\n", name, (void *)script);
#endif

    bsal_lock_unlock(&node->script_lock);
}

int bsal_node_has_script(struct bsal_node *node, struct bsal_script *script)
{
    if (bsal_node_find_script(node, bsal_script_identifier(script)) != NULL) {
        return 1;
    }
    return 0;
}

struct bsal_script *bsal_node_find_script(struct bsal_node *node, int identifier)
{
    struct bsal_script **script;

    script = bsal_map_get(&node->scripts, &identifier);

    if (script == NULL) {
        return NULL;
    }

    return *script;
}

void bsal_node_print_counters(struct bsal_node *node)
{
    printf("----------------------------------------------\n");
    printf("%s Counters for node/%d\n", BSAL_NODE_THORIUM_PREFIX,
                    bsal_node_name(node));
    bsal_counter_print(&node->counter, bsal_node_name(node));
}

/*
 * TODO: on segmentation fault, kill the actor and continue
 * computation
 */
void bsal_node_handle_signal(int signal)
{
    int node;
    struct bsal_node *self;

    self = bsal_node_global_self;

    node = bsal_node_name(self);

    if (signal == SIGSEGV) {
        printf("Error, node/%d received signal SIGSEGV\n", node);

    } else if (signal == SIGUSR1) {
        bsal_node_toggle_debug_mode(bsal_node_global_self);
        return;
    } else {
        printf("Error, node/%d received signal %d\n", node, signal);
    }

    bsal_tracer_print_stack_backtrace();

    fflush(stdout);

    /* remove handler
     * \see http://stackoverflow.com/questions/9302464/how-do-i-remove-a-signal-handler
     */

    self->action.sa_handler = SIG_DFL;
    sigaction(signal, &self->action, NULL);
}

void bsal_node_register_signal_handlers(struct bsal_node *self)
{
    struct bsal_vector signals;
    struct bsal_vector_iterator iterator;
    int *signal;

    bsal_vector_init(&signals, sizeof(int));
    /*
     * \see http://unixhelp.ed.ac.uk/CGI/man-cgi?signal+7
     */
    /* segmentation fault */
    bsal_vector_helper_push_back_int(&signals, SIGSEGV);
    /* division by 0 */
    bsal_vector_helper_push_back_int(&signals, SIGFPE);
    /* bus error (alignment issue */
    bsal_vector_helper_push_back_int(&signals, SIGBUS);
    /* abort */
    bsal_vector_helper_push_back_int(&signals, SIGABRT);

    bsal_vector_helper_push_back_int(&signals, SIGUSR1);

#if 0
    /* interruption */
    bsal_vector_helper_push_back_int(&signals, SIGINT);
    /* kill */
    bsal_vector_helper_push_back_int(&signals, SIGKILL);
    /* termination*/
    bsal_vector_helper_push_back_int(&signals, SIGTERM);
    /* hang up */
    bsal_vector_helper_push_back_int(&signals, SIGHUP);
    /* illegal instruction */
    bsal_vector_helper_push_back_int(&signals, SIGILL);
#endif

    /*
     * \see http://pubs.opengroup.org/onlinepubs/7908799/xsh/sigaction.html
     * \see http://stackoverflow.com/questions/10202941/segmentation-fault-handling
     */
    self->action.sa_handler = bsal_node_handle_signal;
    sigemptyset(&self->action.sa_mask);
    self->action.sa_flags = 0;

    bsal_vector_iterator_init(&iterator, &signals);

    while (bsal_vector_iterator_has_next(&iterator)) {

        bsal_vector_iterator_next(&iterator, (void **)&signal);
        sigaction(*signal, &self->action, NULL);
    }

    bsal_vector_iterator_destroy(&iterator);
    bsal_vector_destroy(&signals);

        /* generate SIGSEGV
    *((int *)NULL) = 0;
     */
}

void bsal_node_print_structure(struct bsal_node *node, struct bsal_actor *actor)
{
    struct bsal_map *structure;
    struct bsal_map_iterator iterator;
    int *source;
    int *count;
    int name;
    int script;
    struct bsal_script *actual_script;
    char color[32];
    int node_name;

    node_name = bsal_node_name(node);
    name = bsal_actor_name(actor);
    script = bsal_actor_script(actor);
    actual_script = bsal_node_find_script(node, script);

    if (node_name == 0) {
        strcpy(color, "red");
    } else if (node_name == 1) {
        strcpy(color, "green");
    } else if (node_name == 2) {
        strcpy(color, "blue");
    } else if (node_name == 3) {
        strcpy(color, "pink");
    }

    structure = bsal_actor_get_received_messages(actor);

    printf("    a%d [label=\"%s/%d\", color=\"%s\"]; /* STRUCTURE vertex */\n", name,
                    bsal_script_description(actual_script), name, color);

    bsal_map_iterator_init(&iterator, structure);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&source, (void **)&count);

        printf("    a%d -> a%d [label=\"%d\"]; /* STRUCTURE edge */\n", *source, name, *count);
    }

    printf(" /* STRUCTURE */\n");

    bsal_map_iterator_destroy(&iterator);
}

struct bsal_worker_pool *bsal_node_get_worker_pool(struct bsal_node *self)
{
    return &self->worker_pool;
}

void bsal_node_toggle_debug_mode(struct bsal_node *self)
{
    self->debug = !self->debug;
    bsal_worker_pool_toggle_debug_mode(&self->worker_pool);
}

void bsal_node_reset_actor_counters(struct bsal_node *node)
{
    struct bsal_map_iterator iterator;
    int *name;
    struct bsal_actor *actor;

    bsal_map_iterator_init(&iterator, &node->actor_names);

    while (bsal_map_iterator_next(&iterator, (void **)&name, NULL)) {

        actor = bsal_node_get_actor_from_name(node, *name);

        bsal_actor_reset_counters(actor);
    }
    bsal_map_iterator_destroy(&iterator);
}

int64_t bsal_node_get_counter(struct bsal_node *node, int counter)
{
    return bsal_counter_get(&node->counter, counter);
}

int bsal_node_send_system(struct bsal_node *node, struct bsal_message *message)
{
    int destination;
    int tag;
    int source;

    tag = bsal_message_tag(message);
    destination = bsal_message_destination(message);
    source = bsal_message_source(message);

    if (source == destination
            && tag == BSAL_ACTOR_ENABLE_AUTO_SCALING) {

        printf("AUTO-SCALING node/%d enables auto-scaling for actor %d (BSAL_ACTOR_ENABLE_AUTO_SCALING)\n",
                       bsal_node_name(node),
                       source);

        bsal_set_add(&node->auto_scaling_actors, &source);

        return 1;

    } else if (source == destination
           && tag == BSAL_ACTOR_DISABLE_AUTO_SCALING) {

        bsal_set_delete(&node->auto_scaling_actors, &source);

        return 1;
    }

    return 0;
}


void bsal_node_send_to_actor(struct bsal_node *node, int name, struct bsal_message *message)
{
    bsal_message_set_source(message, name);
    bsal_message_set_destination(message, name);

    bsal_node_send(node, message);
}


void bsal_node_check_load(struct bsal_node *node)
{
    const float load_threshold = 0.90;
    struct bsal_set_iterator iterator;
    struct bsal_message message;
    int name;
    time_t current_time;

    current_time = time(NULL);

    /* Do the auto-scaling thing one time m aximum
     * for each second
     */
    if (current_time == node->last_auto_scaling) {
        return;
    }

    node->last_auto_scaling = current_time;

    /* Check load to see if auto-scaling is needed
     */

    if (bsal_worker_pool_get_current_load(&node->worker_pool)
                    <= load_threshold) {


        bsal_set_iterator_init(&iterator, &node->auto_scaling_actors);

        while (bsal_set_iterator_get_next_value(&iterator, &name)) {

            bsal_message_init(&message, BSAL_ACTOR_DO_AUTO_SCALING,
                            0, NULL);

            bsal_node_send_to_actor(node, name, &message);

            bsal_message_destroy(&message);
        }

        bsal_set_iterator_destroy(&iterator);
    }
}

int bsal_node_pull(struct bsal_node *node, struct bsal_message *message)
{
    return bsal_worker_pool_dequeue_message(&node->worker_pool, message);
}

void bsal_node_run_loop(struct bsal_node *node)
{
    struct bsal_message message;
    int credits;
    const int starting_credits = 256;

#ifdef BSAL_NODE_ENABLE_INSTRUMENTATION
    int ticks;
    int period;
    time_t current_time;
    char print_information = 0;

    if (node->print_load || node->print_counters) {
        print_information = 1;
    }

    period = BSAL_NODE_LOAD_PERIOD;
    ticks = 0;
#endif

    credits = starting_credits;

    while (credits > 0) {

#ifdef BSAL_NODE_ENABLE_INSTRUMENTATION
        if (print_information) {
            current_time = time(NULL);

            if (current_time - node->last_report_time >= period) {
                if (node->print_load) {
                    bsal_worker_pool_print_load(&node->worker_pool, BSAL_WORKER_POOL_LOAD_EPOCH);

                    /* Display the number of actors,
                     * the number of active buffers/requests/messages,
                     * and
                     * the heap size.
                     */
                    printf("%s node/%d METRICS AliveActorCount: %d ActiveRequestCount: %d HeapByteCount: %" PRIu64 "\n",
                                    BSAL_NODE_THORIUM_PREFIX,
                                    node->name,
                                    node->alive_actors,
                                    bsal_transport_get_active_request_count(&node->transport),
                                    bsal_memory_get_heap_size()
                                    );
                }

                if (node->print_counters) {
                    bsal_node_print_counters(node);
                }

                node->last_report_time = current_time;
            }
        }
#endif

#ifdef BSAL_NODE_DEBUG_LOOP
        if (ticks % 1000000 == 0) {
            bsal_node_print_counters(node);
        }
#endif

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
        if (
#ifdef BSAL_NODE_CHECK_TRANSPORT
            node->use_transport &&
#endif
            bsal_transport_receive(&node->transport, &message)) {

            bsal_counter_add(&node->counter, BSAL_COUNTER_RECEIVED_MESSAGES_NOT_FROM_SELF, 1);
            bsal_counter_add(&node->counter, BSAL_COUNTER_RECEIVED_BYTES_NOT_FROM_SELF,
                    bsal_message_count(&message));


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
        if (!node->send_in_thread) {

#ifdef BSAL_NODE_DEBUG_RUN
            if (node->alive_actors == 0) {
                printf("BSAL_NODE_DEBUG_RUN sending messages, no local actors\n");
            }
#endif
            bsal_node_send_message(node);
        }

#ifdef BSAL_NODE_ENABLE_INSTRUMENTATION
        ticks++;
#endif

        /* Flush queue buffers in the worker pool
         */

        bsal_worker_pool_work(&node->worker_pool);

        --credits;

        /* if the node is still running, allocate new credits
         * to the engine loop
         */
        if (credits == 0) {
            if (bsal_node_running(node)) {
                credits = starting_credits;
#ifdef BSAL_NODE_DEBUG_RUN
                printf("BSAL_NODE_DEBUG_RUN here are some credits\n");
#endif
            }

            bsal_node_check_load(node);
        }

#ifdef BSAL_NODE_DEBUG_RUN
        if (node->alive_actors == 0) {
            printf("BSAL_NODE_DEBUG_RUN credits: %d\n", credits);
        }
#endif
    }

#ifdef BSAL_NODE_DEBUG_20140601_8
    printf("DEBUG node/%d exited loop\n",
                    bsal_node_name(node));
#endif
}

void bsal_node_send_message(struct bsal_node *node)
{
    struct bsal_message message;

#ifdef BSAL_NODE_CHECK_TRANSPORT
    /* Free buffers of active requests
     */
    if (node->use_transport) {
#endif
        bsal_node_test_requests(node);

#ifdef BSAL_NODE_CHECK_TRANSPORT
    }
#endif

    /* check for messages to send from from threads */
    /* this call lock only if there is at least
     * a message in the FIFO
     */
    if (bsal_node_pull(node, &message)) {

        node->last_transport_event_time = time(NULL);

#ifdef BSAL_NODE_DEBUG
        printf("bsal_node_run pulled tag %i buffer %p\n",
                        bsal_message_tag(&message),
                        bsal_message_buffer(&message));
#endif

#ifdef BSAL_NODE_DEBUG_RUN
        if (node->alive_actors == 0) {
            printf("BSAL_NODE_DEBUG_RUN bsal_node_send_message pulled a message, tag %d\n",
                            bsal_message_tag(&message));
        }
#endif

        /* send it locally or over the network */
        bsal_node_send(node, &message);
    } else {
#ifdef BSAL_NODE_DEBUG_RUN
        if (node->alive_actors == 0) {
            printf("BSAL_NODE_DEBUG_RUN bsal_node_send_message no message\n");
        }
#endif

    }
}

void bsal_node_test_requests(struct bsal_node *node)
{
    struct bsal_active_request active_request;
    int requests;
    int requests_to_test;
    int i;

    /*
     * Use a half-life approach
     */
    requests = bsal_transport_get_active_request_count(&node->transport);
    requests_to_test = requests / 2;

    /* Test active buffer requests
     */

    i = 0;
    while (i < requests_to_test) {
        if (bsal_transport_test_requests(&node->transport,
                            &active_request)) {

            bsal_node_free_active_request(node, &active_request);

            bsal_active_request_destroy(&active_request);
        }

        ++i;
    }

#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    /* Check if there are queued buffers to give to workers
     */
    if (bsal_ring_queue_dequeue(&node->outbound_buffers, &active_request)) {

        bsal_node_free_active_request(node, &active_request);
    }
#endif
}

void bsal_node_free_active_request(struct bsal_node *node,
                struct bsal_active_request *active_request)
{
    void *buffer;

#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    int worker_name;
    struct bsal_worker *worker;
#endif

    buffer = bsal_active_request_buffer(active_request);


#ifdef BSAL_NODE_USE_MESSAGE_RECYCLING
    worker_name = bsal_active_request_get_worker(active_request);

    /* This an worker buffer
     */
    if (worker_name >= 0) {
        worker = bsal_worker_pool_get_worker(&node->worker_pool, worker_name);

        /* Push the buffer in the ring of the worker
         */
        if (!bsal_worker_free_buffer(worker, buffer)) {

            /* If the ring is full, queue it locally
             * and try again later
             */
            bsal_ring_queue_enqueue(&node->outbound_buffers, &active_request);
        }

    /* This is a node buffer
     * (for startup)
     */
    }

#endif

    bsal_memory_free(buffer);
}

