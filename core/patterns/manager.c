#include "manager.h"

#include <core/helpers/vector_helper.h>
#include <core/helpers/message_helper.h>

#include <core/structures/vector_iterator.h>
#include <core/structures/map_iterator.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

/*
#define BIOSAL_MANAGER_DEBUG
*/

#define MEMORY_MANAGER 0x0021b5f1

struct thorium_script biosal_manager_script = {
    .identifier = SCRIPT_MANAGER,
    .name = "biosal_manager",
    .description = "Manager",
    .author = "Sebastien Boisvert",
    .version = "",
    .size = sizeof(struct biosal_manager),
    .init = biosal_manager_init,
    .destroy = biosal_manager_destroy,
    .receive = biosal_manager_receive
};

void biosal_manager_init(struct thorium_actor *actor)
{
    struct biosal_manager *concrete_actor;

    concrete_actor = (struct biosal_manager *)thorium_actor_concrete_actor(actor);

    biosal_vector_init(&concrete_actor->children, sizeof(int));

    /*
     * Register the route for stopping
     */

    thorium_actor_add_action(actor, ACTION_ASK_TO_STOP,
                    biosal_manager_ask_to_stop);

    biosal_map_init(&concrete_actor->spawner_child_count, sizeof(int), sizeof(int));
    biosal_map_init(&concrete_actor->spawner_children, sizeof(int), sizeof(struct biosal_vector));
    biosal_vector_init(&concrete_actor->indices, sizeof(int));

    concrete_actor->ready_spawners = 0;
    concrete_actor->spawners = 0;
    concrete_actor->actors_per_spawner = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->actors_per_worker = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->workers_per_actor = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->script = THORIUM_ACTOR_NO_VALUE;
}

void biosal_manager_destroy(struct thorium_actor *actor)
{
    struct biosal_manager *concrete_actor;
    struct biosal_map_iterator iterator;
    struct biosal_vector *vector;

    concrete_actor = (struct biosal_manager *)thorium_actor_concrete_actor(actor);

    biosal_vector_destroy(&concrete_actor->children);

    biosal_map_destroy(&concrete_actor->spawner_child_count);

    biosal_map_iterator_init(&iterator, &concrete_actor->spawner_children);

    while (biosal_map_iterator_has_next(&iterator)) {
        biosal_map_iterator_next(&iterator, NULL, (void **)&vector);

        biosal_vector_destroy(vector);
    }

    biosal_map_iterator_destroy(&iterator);

    biosal_map_destroy(&concrete_actor->spawner_children);

    biosal_vector_destroy(&concrete_actor->indices);
}

void biosal_manager_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    struct biosal_vector spawners;
    struct biosal_vector_iterator iterator;
    int *bucket;
    int index;
    struct biosal_manager *concrete_actor;
    struct biosal_vector *stores;
    int spawner;
    int workers;
    void *buffer;
    int source;
    int store;
    struct biosal_vector all_stores;
    int new_count;
    void *new_buffer;
    struct thorium_message new_message;
    struct biosal_memory_pool *ephemeral_memory;

    if (thorium_actor_take_action(actor, message)) {
        return;
    }

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);
    concrete_actor = (struct biosal_manager *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);

    if (tag == ACTION_START) {

        /* return empty vector
         */
        if (concrete_actor->script == THORIUM_ACTOR_NO_VALUE) {

            biosal_vector_init(&all_stores, sizeof(int));
            thorium_actor_send_reply_vector(actor, ACTION_START_REPLY, &all_stores);
            biosal_vector_destroy(&all_stores);
            return;
        }

#ifdef BIOSAL_MANAGER_DEBUG
        printf("DEBUG manager %d starts\n",
                        thorium_actor_name(actor));
#endif

        biosal_vector_init(&spawners, 0);
        biosal_vector_set_memory_pool(&spawners, ephemeral_memory);
        biosal_vector_unpack(&spawners, buffer);

        concrete_actor->spawners = biosal_vector_size(&spawners);

        printf("DEBUG manager %d starts, supervisor is %d, %d spawners provided\n",
                        thorium_actor_name(actor), thorium_actor_supervisor(actor),
                        (int)biosal_vector_size(&spawners));

        biosal_vector_iterator_init(&iterator, &spawners);

        while (biosal_vector_iterator_has_next(&iterator)) {

            biosal_vector_iterator_next(&iterator, (void **)&bucket);

            spawner = *bucket;
            index = spawner;

            biosal_vector_push_back(&concrete_actor->indices, &index);

            printf("DEBUG manager %d add spawned processes for spawner %d\n",
                            thorium_actor_name(actor), spawner);

            stores = biosal_map_add(&concrete_actor->spawner_children, &index);

#ifdef BIOSAL_MANAGER_DEBUG
            printf("DEBUG adding %d to table\n", index);
#endif

            bucket = biosal_map_add(&concrete_actor->spawner_child_count, &index);
            *bucket = 0;

#ifdef BIOSAL_MANAGER_DEBUG
            printf("DEBUG685-1 spawner %d index %d bucket %p\n", spawner, index, (void *)bucket);
            biosal_vector_print_int(thorium_actor_acquaintance_vector(actor));
#endif

            biosal_vector_init(stores, sizeof(int));

            thorium_actor_send_empty(actor, spawner, ACTION_GET_NODE_WORKER_COUNT);
        }

        biosal_vector_iterator_destroy(&iterator);
        biosal_vector_destroy(&spawners);

    } else if (tag == ACTION_MANAGER_SET_SCRIPT) {

        concrete_actor->script = *(int *)buffer;

#ifdef BIOSAL_MANAGER_DEBUG
        BIOSAL_DEBUG_MARKER("set_the_script_now");
#endif

        printf("manager %d sets script to script %x\n",
                        thorium_actor_name(actor), concrete_actor->script);

        thorium_actor_send_reply_empty(actor, ACTION_MANAGER_SET_SCRIPT_REPLY);

#ifdef BIOSAL_MANAGER_DEBUG
        BIOSAL_DEBUG_MARKER("manager sends reply");
#endif

    } else if (tag == ACTION_MANAGER_SET_ACTORS_PER_SPAWNER) {
        concrete_actor->actors_per_spawner = *(int *)buffer;

        if (concrete_actor->actors_per_spawner <= 0) {
            concrete_actor->actors_per_spawner = THORIUM_ACTOR_NO_VALUE;
        }

        thorium_actor_send_reply_empty(actor, ACTION_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY);

    } else if (tag == ACTION_GET_NODE_WORKER_COUNT_REPLY) {

        thorium_message_unpack_int(message, 0, &workers);

        index = source;

        printf("DEBUG manager %d says that spawner %d is on a node with %d workers\n",
                        thorium_actor_name(actor), source, workers);

#ifdef BIOSAL_MANAGER_DEBUG
        printf("DEBUG getting table index %d\n", index);
#endif

        bucket = biosal_map_get(&concrete_actor->spawner_child_count, &index);

#ifdef BIOSAL_MANAGER_DEBUG
        printf("DEBUG685-2 spawner %d index %d bucket %p\n", source, index, (void *)bucket);
        biosal_vector_print_int(thorium_actor_acquaintance_vector(actor));
#endif

        /* Option 1: Use a number of actors for each spawner. This number
         * is the same for all spawner.
         */
        if (concrete_actor->actors_per_spawner != THORIUM_ACTOR_NO_VALUE) {
            (*bucket) = concrete_actor->actors_per_spawner;

        /* Otherwise, the number of actors is either a number of actors per worker
         * or a number of workers per actor (one of the two options).
         */

        /* Option 2: spawn 1 actor for each N workers
         */
        } else if (concrete_actor->workers_per_actor != THORIUM_ACTOR_NO_VALUE) {

            (*bucket) = workers / concrete_actor->workers_per_actor;

            if (workers % concrete_actor->workers_per_actor != 0) {
                ++(*bucket);
            }

        /* Option 3: Otherwise, multiply the number of workers by a number of actors
         * per worker.
         */
        } else if (concrete_actor->actors_per_worker != THORIUM_ACTOR_NO_VALUE) {

            (*bucket) = workers * concrete_actor->actors_per_worker;

        /* Option 4: Otherwise, spawn 1 actor per worker for each spawner.
         */
        } else {

            (*bucket) = workers * 1;

        }

        thorium_actor_send_reply_int(actor, ACTION_SPAWN, concrete_actor->script);

    } else if (tag == ACTION_SPAWN_REPLY) {

        store = *(int *)buffer;
        index = source;

        stores = biosal_map_get(&concrete_actor->spawner_children, &index);
        bucket = biosal_map_get(&concrete_actor->spawner_child_count, &index);

        biosal_vector_push_back(stores, &store);

#ifdef BIOSAL_MANAGER_DEBUG
        printf("DEBUG manager %d receives %d from spawner %d, now %d/%d\n",
                        thorium_actor_name(actor), store, source,
                        (int)biosal_vector_size(stores), *bucket);
#endif

        if (biosal_vector_size(stores) >= *bucket) {

            concrete_actor->ready_spawners++;

            printf("DEBUG manager %d says that spawner %d is ready, %d/%d (spawned %d actors)\n",
                        thorium_actor_name(actor), source,
                        concrete_actor->ready_spawners, concrete_actor->spawners,
                        (int)biosal_vector_size(stores));

            if (concrete_actor->ready_spawners == concrete_actor->spawners) {

                printf("DEBUG manager %d says that all spawners are ready\n",
                        thorium_actor_name(actor));

                biosal_vector_init(&all_stores, sizeof(int));

                biosal_vector_iterator_init(&iterator, &concrete_actor->indices);

                while (biosal_vector_iterator_has_next(&iterator)) {
                    biosal_vector_iterator_next(&iterator, (void **)&bucket);

                    index = *bucket;

                    stores = (struct biosal_vector *)biosal_map_get(&concrete_actor->spawner_children,
                                    &index);

                    biosal_vector_push_back_vector(&all_stores, stores);
                }

                biosal_vector_iterator_destroy(&iterator);

                new_count = biosal_vector_pack_size(&all_stores);
                new_buffer = biosal_memory_allocate(new_count, MEMORY_MANAGER);
                biosal_vector_pack(&all_stores, new_buffer);

                /*
                 * Save the list of actors for later
                 */
                biosal_vector_push_back_vector(&concrete_actor->children,
                                &all_stores);

                biosal_vector_destroy(&all_stores);

                thorium_message_init(&new_message, ACTION_START_REPLY, new_count, new_buffer);
                thorium_actor_send_to_supervisor(actor, &new_message);

                biosal_memory_free(new_buffer, MEMORY_MANAGER);

                thorium_message_destroy(&new_message);
            }
        } else {

            thorium_actor_send_reply_int(actor, ACTION_SPAWN, concrete_actor->script);
        }


    } else if (tag == ACTION_MANAGER_SET_ACTORS_PER_WORKER) {

        thorium_message_unpack_int(message, 0, &concrete_actor->actors_per_worker);

        if (concrete_actor->actors_per_worker <= 0) {
            concrete_actor->actors_per_worker = THORIUM_ACTOR_NO_VALUE;
        }

        thorium_actor_send_reply_empty(actor, ACTION_MANAGER_SET_ACTORS_PER_WORKER_REPLY);

    } else if (tag == ACTION_MANAGER_SET_WORKERS_PER_ACTOR) {

        thorium_message_unpack_int(message, 0, &concrete_actor->workers_per_actor);

        if (concrete_actor->workers_per_actor <= 0) {
            concrete_actor->workers_per_actor = THORIUM_ACTOR_NO_VALUE;
        }

        thorium_actor_send_reply_empty(actor, ACTION_MANAGER_SET_WORKERS_PER_ACTOR_REPLY);

    }
}

void biosal_manager_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message)
{

    struct biosal_manager *concrete_actor;
    int i;
    int size;
    int child;

    printf("%s/%d dies\n",
                    thorium_actor_script_name(actor),
                    thorium_actor_name(actor));

    concrete_actor = (struct biosal_manager *)thorium_actor_concrete_actor(actor);
    thorium_actor_ask_to_stop(actor, message);

    /*
     * Stop children too
     */

    size = biosal_vector_size(&concrete_actor->children);

    for (i = 0; i < size; i++) {

        child = biosal_vector_at_as_int(&concrete_actor->children, i);

        thorium_actor_send_empty(actor, child, ACTION_ASK_TO_STOP);
    }
}
