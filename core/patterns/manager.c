#include "manager.h"

#include <core/helpers/actor_helper.h>
#include <core/helpers/vector_helper.h>
#include <core/helpers/message_helper.h>

#include <core/structures/vector_iterator.h>
#include <core/structures/dynamic_hash_table_iterator.h>

#include <core/system/debugger.h>
#include <core/system/memory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

/*
#define BSAL_MANAGER_DEBUG
*/

struct bsal_script bsal_manager_script = {
    .name = BSAL_MANAGER_SCRIPT,
    .init = bsal_manager_init,
    .destroy = bsal_manager_destroy,
    .receive = bsal_manager_receive,
    .size = sizeof(struct bsal_manager),
    .description = "manager"
};

void bsal_manager_init(struct bsal_actor *actor)
{
    struct bsal_manager *concrete_actor;

    concrete_actor = (struct bsal_manager *)bsal_actor_concrete_actor(actor);

    bsal_dynamic_hash_table_init(&concrete_actor->spawner_child_count, 128, sizeof(int), sizeof(int));
    bsal_dynamic_hash_table_init(&concrete_actor->spawner_children, 128, sizeof(int), sizeof(struct bsal_vector));
    bsal_vector_init(&concrete_actor->indices, sizeof(int));

    concrete_actor->ready_spawners = 0;
    concrete_actor->spawners = 0;
    concrete_actor->actors_per_spawner = BSAL_ACTOR_NO_VALUE;
    concrete_actor->actors_per_worker = BSAL_ACTOR_NO_VALUE;
    concrete_actor->workers_per_actor = BSAL_ACTOR_NO_VALUE;
    concrete_actor->script = BSAL_ACTOR_NO_VALUE;
}

void bsal_manager_destroy(struct bsal_actor *actor)
{
    struct bsal_manager *concrete_actor;
    struct bsal_dynamic_hash_table_iterator iterator;
    struct bsal_vector *vector;

    concrete_actor = (struct bsal_manager *)bsal_actor_concrete_actor(actor);

    bsal_dynamic_hash_table_destroy(&concrete_actor->spawner_child_count);

    bsal_dynamic_hash_table_iterator_init(&iterator, &concrete_actor->spawner_children);

    while (bsal_dynamic_hash_table_iterator_has_next(&iterator)) {
        bsal_dynamic_hash_table_iterator_next(&iterator, NULL, (void **)&vector);

        bsal_vector_destroy(vector);
    }

    bsal_dynamic_hash_table_iterator_destroy(&iterator);

    bsal_dynamic_hash_table_destroy(&concrete_actor->spawner_children);

    bsal_vector_destroy(&concrete_actor->indices);
}

void bsal_manager_receive(struct bsal_actor *actor, struct bsal_message *message)
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
    struct bsal_message new_message;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(actor);
    source = bsal_message_source(message);
    buffer = bsal_message_buffer(message);
    concrete_actor = (struct bsal_manager *)bsal_actor_concrete_actor(actor);
    tag = bsal_message_tag(message);

    if (tag == BSAL_ACTOR_START) {

        /* return empty vector
         */
        if (concrete_actor->script == BSAL_ACTOR_NO_VALUE) {

            bsal_vector_init(&all_stores, sizeof(int));
            bsal_actor_helper_send_reply_vector(actor, BSAL_ACTOR_START_REPLY, &all_stores);
            bsal_vector_destroy(&all_stores);
            return;
        }

#ifdef BSAL_MANAGER_DEBUG
        printf("DEBUG manager %d starts\n",
                        bsal_actor_name(actor));
#endif

        bsal_vector_init(&spawners, 0);
        bsal_vector_set_memory_pool(&spawners, ephemeral_memory);
        bsal_vector_unpack(&spawners, buffer);

        concrete_actor->spawners = bsal_vector_size(&spawners);

        printf("DEBUG manager %d starts, supervisor is %d, %d spawners provided\n",
                        bsal_actor_name(actor), bsal_actor_supervisor(actor),
                        (int)bsal_vector_size(&spawners));

        bsal_vector_iterator_init(&iterator, &spawners);

        while (bsal_vector_iterator_has_next(&iterator)) {

            bsal_vector_iterator_next(&iterator, (void **)&bucket);

            spawner = *bucket;
            index = bsal_actor_add_acquaintance(actor, spawner);

            bsal_vector_push_back(&concrete_actor->indices, &index);

            printf("DEBUG manager %d add spawned processes for spawner %d\n",
                            bsal_actor_name(actor), spawner);

            stores = (struct bsal_vector *)bsal_dynamic_hash_table_add(&concrete_actor->spawner_children, &index);

#ifdef BSAL_MANAGER_DEBUG
            printf("DEBUG adding %d to table\n", index);
#endif

            bucket = (int *)bsal_dynamic_hash_table_add(&concrete_actor->spawner_child_count, &index);
            *bucket = 0;

#ifdef BSAL_MANAGER_DEBUG
            printf("DEBUG685-1 spawner %d index %d bucket %p\n", spawner, index, (void *)bucket);
            bsal_vector_helper_print_int(bsal_actor_acquaintance_vector(actor));
#endif

            bsal_vector_init(stores, sizeof(int));

            bsal_actor_helper_send_empty(actor, spawner, BSAL_ACTOR_GET_NODE_WORKER_COUNT);
        }

        bsal_vector_iterator_destroy(&iterator);
        bsal_vector_destroy(&spawners);

    } else if (tag == BSAL_MANAGER_SET_SCRIPT) {

        concrete_actor->script = *(int *)buffer;

#ifdef BSAL_MANAGER_DEBUG
        BSAL_DEBUG_MARKER("set_the_script_now");
#endif

        printf("manager %d sets script to script %x\n",
                        bsal_actor_name(actor), concrete_actor->script);

        bsal_actor_helper_send_reply_empty(actor, BSAL_MANAGER_SET_SCRIPT_REPLY);

#ifdef BSAL_MANAGER_DEBUG
        BSAL_DEBUG_MARKER("manager sends reply");
#endif

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_SPAWNER) {
        concrete_actor->actors_per_spawner = *(int *)buffer;

        if (concrete_actor->actors_per_spawner <= 0) {
            concrete_actor->actors_per_spawner = BSAL_ACTOR_NO_VALUE;
        }

        bsal_actor_helper_send_reply_empty(actor, BSAL_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY);

    } else if (tag == BSAL_ACTOR_GET_NODE_WORKER_COUNT_REPLY) {

        workers = *(int *)buffer;

        index = bsal_actor_get_acquaintance_index(actor, source);

        printf("DEBUG manager %d says that spawner %d is on a node with %d workers\n",
                        bsal_actor_name(actor), source, workers);

#ifdef BSAL_MANAGER_DEBUG
        printf("DEBUG getting table index %d\n", index);
#endif

        bucket = (int *)bsal_dynamic_hash_table_get(&concrete_actor->spawner_child_count, &index);

#ifdef BSAL_MANAGER_DEBUG
        printf("DEBUG685-2 spawner %d index %d bucket %p\n", source, index, (void *)bucket);
        bsal_vector_helper_print_int(bsal_actor_acquaintance_vector(actor));
#endif

        /* Option 1: Use a number of actors for each spawner. This number
         * is the same for all spawner.
         */
        if (concrete_actor->actors_per_spawner != BSAL_ACTOR_NO_VALUE) {
            (*bucket) = concrete_actor->actors_per_spawner;

        /* Otherwise, the number of actors is either a number of actors per worker
         * or a number of workers per actor (one of the two options).
         */

        /* Option 2: spawn 1 actor for each N workers
         */
        } else if (concrete_actor->workers_per_actor != BSAL_ACTOR_NO_VALUE) {

            (*bucket) = workers / concrete_actor->workers_per_actor;

            if (workers % concrete_actor->workers_per_actor != 0) {
                ++(*bucket);
            }

        /* Option 3: Otherwise, multiply the number of workers by a number of actors
         * per worker.
         */
        } else if (concrete_actor->actors_per_worker != BSAL_ACTOR_NO_VALUE) {

            (*bucket) = workers * concrete_actor->actors_per_worker;

        /* Option 4: Otherwise, spawn 1 actor per worker for each spawner.
         */
        } else {

            (*bucket) = workers * 1;

        }

        bsal_actor_helper_send_reply_int(actor, BSAL_ACTOR_SPAWN, concrete_actor->script);

    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        store = *(int *)buffer;
        index = bsal_actor_get_acquaintance_index(actor, source);

        stores = (struct bsal_vector *)bsal_dynamic_hash_table_get(&concrete_actor->spawner_children, &index);
        bucket = (int *)bsal_dynamic_hash_table_get(&concrete_actor->spawner_child_count, &index);

        bsal_vector_push_back(stores, &store);

#ifdef BSAL_MANAGER_DEBUG
        printf("DEBUG manager %d receives %d from spawner %d, now %d/%d\n",
                        bsal_actor_name(actor), store, source,
                        (int)bsal_vector_size(stores), *bucket);
#endif

        if (bsal_vector_size(stores) == *bucket) {

            concrete_actor->ready_spawners++;

            printf("DEBUG manager %d says that spawner %d is ready, %d/%d\n",
                        bsal_actor_name(actor), source,
                        concrete_actor->ready_spawners, concrete_actor->spawners);

            if (concrete_actor->ready_spawners == concrete_actor->spawners) {

                printf("DEBUG manager %d says that all spawners are ready\n",
                        bsal_actor_name(actor));

                bsal_vector_init(&all_stores, sizeof(int));

                bsal_vector_iterator_init(&iterator, &concrete_actor->indices);

                while (bsal_vector_iterator_has_next(&iterator)) {
                    bsal_vector_iterator_next(&iterator, (void **)&bucket);

                    index = *bucket;

                    stores = (struct bsal_vector *)bsal_dynamic_hash_table_get(&concrete_actor->spawner_children,
                                    &index);

                    bsal_vector_push_back_vector(&all_stores, stores);
                }

                bsal_vector_iterator_destroy(&iterator);

                new_count = bsal_vector_pack_size(&all_stores);
                new_buffer = bsal_memory_allocate(new_count);
                bsal_vector_pack(&all_stores, new_buffer);
                bsal_vector_destroy(&all_stores);

                bsal_message_init(&new_message, BSAL_ACTOR_START_REPLY, new_count, new_buffer);
                bsal_actor_send_to_supervisor(actor, &new_message);

                bsal_memory_free(new_buffer);

                bsal_message_destroy(&new_message);
            }
        } else {

            bsal_actor_helper_send_reply_int(actor, BSAL_ACTOR_SPAWN, concrete_actor->script);
        }

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("manager %d dies\n", bsal_actor_name(actor));

        bsal_actor_helper_ask_to_stop(actor, message);

    } else if (tag == BSAL_MANAGER_SET_ACTORS_PER_WORKER) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->actors_per_worker);

        if (concrete_actor->actors_per_worker <= 0) {
            concrete_actor->actors_per_worker = BSAL_ACTOR_NO_VALUE;
        }

        bsal_actor_helper_send_reply_empty(actor, BSAL_MANAGER_SET_ACTORS_PER_WORKER_REPLY);

    } else if (tag == BSAL_MANAGER_SET_WORKERS_PER_ACTOR) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->workers_per_actor);

        if (concrete_actor->workers_per_actor <= 0) {
            concrete_actor->workers_per_actor = BSAL_ACTOR_NO_VALUE;
        }

        bsal_actor_helper_send_reply_empty(actor, BSAL_MANAGER_SET_WORKERS_PER_ACTOR_REPLY);

    }
}
