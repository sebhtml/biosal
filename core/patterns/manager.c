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
#define BSAL_MANAGER_DEBUG
*/

struct thorium_script bsal_manager_script = {
    .identifier = SCRIPT_MANAGER,
    .name = "bsal_manager",
    .description = "Manager",
    .author = "Sebastien Boisvert",
    .version = "",
    .size = sizeof(struct bsal_manager),
    .init = bsal_manager_init,
    .destroy = bsal_manager_destroy,
    .receive = bsal_manager_receive
};

void bsal_manager_init(struct thorium_actor *actor)
{
    struct bsal_manager *concrete_actor;

    concrete_actor = (struct bsal_manager *)thorium_actor_concrete_actor(actor);

    bsal_vector_init(&concrete_actor->children, sizeof(int));

    /*
     * Register the route for stopping
     */

    thorium_actor_add_action(actor, THORIUM_ACTOR_ASK_TO_STOP,
                    bsal_manager_ask_to_stop);

    bsal_map_init(&concrete_actor->spawner_child_count, sizeof(int), sizeof(int));
    bsal_map_init(&concrete_actor->spawner_children, sizeof(int), sizeof(struct bsal_vector));
    bsal_vector_init(&concrete_actor->indices, sizeof(int));

    concrete_actor->ready_spawners = 0;
    concrete_actor->spawners = 0;
    concrete_actor->actors_per_spawner = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->actors_per_worker = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->workers_per_actor = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->script = THORIUM_ACTOR_NO_VALUE;
}

void bsal_manager_destroy(struct thorium_actor *actor)
{
    struct bsal_manager *concrete_actor;
    struct bsal_map_iterator iterator;
    struct bsal_vector *vector;

    concrete_actor = (struct bsal_manager *)thorium_actor_concrete_actor(actor);

    bsal_vector_destroy(&concrete_actor->children);

    bsal_map_destroy(&concrete_actor->spawner_child_count);

    bsal_map_iterator_init(&iterator, &concrete_actor->spawner_children);

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, NULL, (void **)&vector);

        bsal_vector_destroy(vector);
    }

    bsal_map_iterator_destroy(&iterator);

    bsal_map_destroy(&concrete_actor->spawner_children);

    bsal_vector_destroy(&concrete_actor->indices);
}

void bsal_manager_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    struct bsal_vector spawners;
    struct bsal_vector_iterator iterator;
    int *bucket;
    int index;
    struct bsal_manager *concrete_actor;
    struct bsal_vector *stores;
    int spawner;
    int workers;
    void *buffer;
    int source;
    int store;
    struct bsal_vector all_stores;
    int new_count;
    void *new_buffer;
    struct thorium_message new_message;
    struct bsal_memory_pool *ephemeral_memory;

    if (thorium_actor_use_route(actor, message)) {
        return;
    }

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);
    concrete_actor = (struct bsal_manager *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_tag(message);

    if (tag == THORIUM_ACTOR_START) {

        /* return empty vector
         */
        if (concrete_actor->script == THORIUM_ACTOR_NO_VALUE) {

            bsal_vector_init(&all_stores, sizeof(int));
            thorium_actor_send_reply_vector(actor, THORIUM_ACTOR_START_REPLY, &all_stores);
            bsal_vector_destroy(&all_stores);
            return;
        }

#ifdef BSAL_MANAGER_DEBUG
        printf("DEBUG manager %d starts\n",
                        thorium_actor_name(actor));
#endif

        bsal_vector_init(&spawners, 0);
        bsal_vector_set_memory_pool(&spawners, ephemeral_memory);
        bsal_vector_unpack(&spawners, buffer);

        concrete_actor->spawners = bsal_vector_size(&spawners);

        printf("DEBUG manager %d starts, supervisor is %d, %d spawners provided\n",
                        thorium_actor_name(actor), thorium_actor_supervisor(actor),
                        (int)bsal_vector_size(&spawners));

        bsal_vector_iterator_init(&iterator, &spawners);

        while (bsal_vector_iterator_has_next(&iterator)) {

            bsal_vector_iterator_next(&iterator, (void **)&bucket);

            spawner = *bucket;
            index = spawner;

            bsal_vector_push_back(&concrete_actor->indices, &index);

            printf("DEBUG manager %d add spawned processes for spawner %d\n",
                            thorium_actor_name(actor), spawner);

            stores = bsal_map_add(&concrete_actor->spawner_children, &index);

#ifdef BSAL_MANAGER_DEBUG
            printf("DEBUG adding %d to table\n", index);
#endif

            bucket = bsal_map_add(&concrete_actor->spawner_child_count, &index);
            *bucket = 0;

#ifdef BSAL_MANAGER_DEBUG
            printf("DEBUG685-1 spawner %d index %d bucket %p\n", spawner, index, (void *)bucket);
            bsal_vector_print_int(thorium_actor_acquaintance_vector(actor));
#endif

            bsal_vector_init(stores, sizeof(int));

            thorium_actor_send_empty(actor, spawner, THORIUM_ACTOR_GET_NODE_WORKER_COUNT);
        }

        bsal_vector_iterator_destroy(&iterator);
        bsal_vector_destroy(&spawners);

    } else if (tag == BSAL_MANAGER_SET_SCRIPT) {

        concrete_actor->script = *(int *)buffer;

#ifdef BSAL_MANAGER_DEBUG
        BSAL_DEBUG_MARKER("set_the_script_now");
#endif

        printf("manager %d sets script to script %x\n",
                        thorium_actor_name(actor), concrete_actor->script);

        thorium_actor_send_reply_empty(actor, BSAL_MANAGER_SET_SCRIPT_REPLY);

#ifdef BSAL_MANAGER_DEBUG
        BSAL_DEBUG_MARKER("manager sends reply");
#endif

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_SPAWNER) {
        concrete_actor->actors_per_spawner = *(int *)buffer;

        if (concrete_actor->actors_per_spawner <= 0) {
            concrete_actor->actors_per_spawner = THORIUM_ACTOR_NO_VALUE;
        }

        thorium_actor_send_reply_empty(actor, BSAL_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY);

    } else if (tag == THORIUM_ACTOR_GET_NODE_WORKER_COUNT_REPLY) {

        thorium_message_unpack_int(message, 0, &workers);

        index = source;

        printf("DEBUG manager %d says that spawner %d is on a node with %d workers\n",
                        thorium_actor_name(actor), source, workers);

#ifdef BSAL_MANAGER_DEBUG
        printf("DEBUG getting table index %d\n", index);
#endif

        bucket = bsal_map_get(&concrete_actor->spawner_child_count, &index);

#ifdef BSAL_MANAGER_DEBUG
        printf("DEBUG685-2 spawner %d index %d bucket %p\n", source, index, (void *)bucket);
        bsal_vector_print_int(thorium_actor_acquaintance_vector(actor));
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

        thorium_actor_send_reply_int(actor, THORIUM_ACTOR_SPAWN, concrete_actor->script);

    } else if (tag == THORIUM_ACTOR_SPAWN_REPLY) {

        store = *(int *)buffer;
        index = source;

        stores = bsal_map_get(&concrete_actor->spawner_children, &index);
        bucket = bsal_map_get(&concrete_actor->spawner_child_count, &index);

        bsal_vector_push_back(stores, &store);

#ifdef BSAL_MANAGER_DEBUG
        printf("DEBUG manager %d receives %d from spawner %d, now %d/%d\n",
                        thorium_actor_name(actor), store, source,
                        (int)bsal_vector_size(stores), *bucket);
#endif

        if (bsal_vector_size(stores) >= *bucket) {

            concrete_actor->ready_spawners++;

            printf("DEBUG manager %d says that spawner %d is ready, %d/%d (spawned %d actors)\n",
                        thorium_actor_name(actor), source,
                        concrete_actor->ready_spawners, concrete_actor->spawners,
                        (int)bsal_vector_size(stores));

            if (concrete_actor->ready_spawners == concrete_actor->spawners) {

                printf("DEBUG manager %d says that all spawners are ready\n",
                        thorium_actor_name(actor));

                bsal_vector_init(&all_stores, sizeof(int));

                bsal_vector_iterator_init(&iterator, &concrete_actor->indices);

                while (bsal_vector_iterator_has_next(&iterator)) {
                    bsal_vector_iterator_next(&iterator, (void **)&bucket);

                    index = *bucket;

                    stores = (struct bsal_vector *)bsal_map_get(&concrete_actor->spawner_children,
                                    &index);

                    bsal_vector_push_back_vector(&all_stores, stores);
                }

                bsal_vector_iterator_destroy(&iterator);

                new_count = bsal_vector_pack_size(&all_stores);
                new_buffer = bsal_memory_allocate(new_count);
                bsal_vector_pack(&all_stores, new_buffer);

                /*
                 * Save the list of actors for later
                 */
                bsal_vector_push_back_vector(&concrete_actor->children,
                                &all_stores);

                bsal_vector_destroy(&all_stores);

                thorium_message_init(&new_message, THORIUM_ACTOR_START_REPLY, new_count, new_buffer);
                thorium_actor_send_to_supervisor(actor, &new_message);

                bsal_memory_free(new_buffer);

                thorium_message_destroy(&new_message);
            }
        } else {

            thorium_actor_send_reply_int(actor, THORIUM_ACTOR_SPAWN, concrete_actor->script);
        }


    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_WORKER) {

        thorium_message_unpack_int(message, 0, &concrete_actor->actors_per_worker);

        if (concrete_actor->actors_per_worker <= 0) {
            concrete_actor->actors_per_worker = THORIUM_ACTOR_NO_VALUE;
        }

        thorium_actor_send_reply_empty(actor, BSAL_MANAGER_SET_ACTORS_PER_WORKER_REPLY);

    } else if (tag == BSAL_MANAGER_SET_WORKERS_PER_ACTOR) {

        thorium_message_unpack_int(message, 0, &concrete_actor->workers_per_actor);

        if (concrete_actor->workers_per_actor <= 0) {
            concrete_actor->workers_per_actor = THORIUM_ACTOR_NO_VALUE;
        }

        thorium_actor_send_reply_empty(actor, BSAL_MANAGER_SET_WORKERS_PER_ACTOR_REPLY);

    }
}

void bsal_manager_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message)
{

    struct bsal_manager *concrete_actor;
    int i;
    int size;
    int child;

    printf("%s/%d dies\n",
                    thorium_actor_script_name(actor),
                    thorium_actor_name(actor));

    concrete_actor = (struct bsal_manager *)thorium_actor_concrete_actor(actor);
    thorium_actor_ask_to_stop(actor, message);

    /*
     * Stop children too
     */

    size = bsal_vector_size(&concrete_actor->children);

    for (i = 0; i < size; i++) {

        child = bsal_vector_at_as_int(&concrete_actor->children, i);

        thorium_actor_send_empty(actor, child, THORIUM_ACTOR_ASK_TO_STOP);
    }
}
