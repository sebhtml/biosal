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
#define CORE_MANAGER_DEBUG
*/

#define MEMORY_MANAGER 0x0021b5f1

/*
 * Use ACTION_SPAWN_MANY to avoid sending too many messages.
 */
#define USE_ACTION_SPAWN_MANY

void core_manager_init(struct thorium_actor *self);
void core_manager_destroy(struct thorium_actor *self);
void core_manager_receive(struct thorium_actor *self, struct thorium_message *message);

void core_manager_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message);

struct thorium_script core_manager_script = {
    .identifier = SCRIPT_MANAGER,
    .name = "core_manager",
    .description = "Manager",
    .author = "Sebastien Boisvert",
    .version = "",
    .size = sizeof(struct core_manager),
    .init = core_manager_init,
    .destroy = core_manager_destroy,
    .receive = core_manager_receive
};

void core_manager_init(struct thorium_actor *actor)
{
    struct core_manager *concrete_actor;

    concrete_actor = (struct core_manager *)thorium_actor_concrete_actor(actor);

    core_vector_init(&concrete_actor->children, sizeof(int));

    /*
     * Register the route for stopping
     */

    thorium_actor_add_action(actor, ACTION_ASK_TO_STOP,
                    core_manager_ask_to_stop);

    core_map_init(&concrete_actor->spawner_child_count, sizeof(int), sizeof(int));
    core_map_init(&concrete_actor->spawner_children, sizeof(int), sizeof(struct core_vector));
    core_vector_init(&concrete_actor->indices, sizeof(int));
    core_vector_set_memory_pool(&concrete_actor->indices,
                    thorium_actor_get_persistent_memory_pool(actor));

    concrete_actor->ready_spawners = 0;
    concrete_actor->spawners = 0;
    concrete_actor->actors_per_spawner = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->actors_per_worker = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->workers_per_actor = THORIUM_ACTOR_NO_VALUE;
    concrete_actor->script = THORIUM_ACTOR_NO_VALUE;
}

void core_manager_destroy(struct thorium_actor *actor)
{
    struct core_manager *concrete_actor;
    struct core_map_iterator iterator;
    struct core_vector *vector;

    concrete_actor = (struct core_manager *)thorium_actor_concrete_actor(actor);

    core_vector_destroy(&concrete_actor->children);

    core_map_destroy(&concrete_actor->spawner_child_count);

    core_map_iterator_init(&iterator, &concrete_actor->spawner_children);

    while (core_map_iterator_has_next(&iterator)) {
        core_map_iterator_next(&iterator, NULL, (void **)&vector);

        core_vector_destroy(vector);
    }

    core_map_iterator_destroy(&iterator);

    core_map_destroy(&concrete_actor->spawner_children);

    core_vector_destroy(&concrete_actor->indices);
}

void core_manager_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    struct core_vector spawners;
    struct core_vector_iterator iterator;
    int *bucket;
    int index;
    struct core_manager *concrete_actor;
    struct core_vector *stores;
    int spawner;
    int workers;
    void *buffer;
    int source;
    int store;
    struct core_vector all_stores;
    int new_count;
    char *new_buffer;
    struct thorium_message new_message;
    struct core_memory_pool *ephemeral_memory;
    struct core_vector received_actors;
    char use_spawn_many;
    int offset;

    if (thorium_actor_take_action(actor, message)) {
        return;
    }

    ephemeral_memory = thorium_actor_get_ephemeral_memory(actor);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);
    concrete_actor = (struct core_manager *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);

    if (tag == ACTION_START) {

        /* return empty vector
         */
        if (concrete_actor->script == THORIUM_ACTOR_NO_VALUE) {

            core_vector_init(&all_stores, sizeof(int));
            thorium_actor_send_reply_vector(actor, ACTION_START_REPLY, &all_stores);
            core_vector_destroy(&all_stores);
            return;
        }

#ifdef CORE_MANAGER_DEBUG
        printf("DEBUG manager %d starts\n",
                        thorium_actor_name(actor));
#endif

        core_vector_init(&spawners, 0);
        core_vector_set_memory_pool(&spawners, ephemeral_memory);
        core_vector_unpack(&spawners, buffer);

        concrete_actor->spawners = core_vector_size(&spawners);

        printf("DEBUG manager %d starts, supervisor is %d, %d spawners provided\n",
                        thorium_actor_name(actor), thorium_actor_supervisor(actor),
                        (int)core_vector_size(&spawners));

        core_vector_iterator_init(&iterator, &spawners);

        while (core_vector_iterator_has_next(&iterator)) {

            core_vector_iterator_next(&iterator, (void **)&bucket);

            spawner = *bucket;
            index = spawner;

            core_vector_push_back(&concrete_actor->indices, &index);

            printf("DEBUG manager %d add spawned processes for spawner %d\n",
                            thorium_actor_name(actor), spawner);

            stores = core_map_add(&concrete_actor->spawner_children, &index);

#ifdef CORE_MANAGER_DEBUG
            printf("DEBUG adding %d to table\n", index);
#endif

            bucket = core_map_add(&concrete_actor->spawner_child_count, &index);
            *bucket = 0;

#ifdef CORE_MANAGER_DEBUG
            printf("DEBUG685-1 spawner %d index %d bucket %p\n", spawner, index, (void *)bucket);
            core_vector_print_int(thorium_actor_acquaintance_vector(actor));
#endif

            core_vector_init(stores, sizeof(int));

            thorium_actor_send_empty(actor, spawner, ACTION_GET_NODE_WORKER_COUNT);
        }

        core_vector_iterator_destroy(&iterator);
        core_vector_destroy(&spawners);

    } else if (tag == ACTION_MANAGER_SET_SCRIPT) {

        concrete_actor->script = *(int *)buffer;

#ifdef CORE_MANAGER_DEBUG
        CORE_DEBUG_MARKER("set_the_script_now");
#endif

        printf("manager %d sets script to script %x\n",
                        thorium_actor_name(actor), concrete_actor->script);

        thorium_actor_send_reply_empty(actor, ACTION_MANAGER_SET_SCRIPT_REPLY);

#ifdef CORE_MANAGER_DEBUG
        CORE_DEBUG_MARKER("manager sends reply");
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

#ifdef CORE_MANAGER_DEBUG
        printf("DEBUG getting table index %d\n", index);
#endif

        bucket = core_map_get(&concrete_actor->spawner_child_count, &index);

#ifdef CORE_MANAGER_DEBUG
        printf("DEBUG685-2 spawner %d index %d bucket %p\n", source, index, (void *)bucket);
        core_vector_print_int(thorium_actor_acquaintance_vector(actor));
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

#ifdef USE_ACTION_SPAWN_MANY
        use_spawn_many = 1;
#else
        use_spawn_many = 0;
#endif

        if (use_spawn_many) {
            new_buffer = thorium_actor_allocate(actor, 2 * sizeof(int));
            offset = 0;

            core_memory_copy(new_buffer + 0, &concrete_actor->script, sizeof(concrete_actor->script));
            offset += sizeof(concrete_actor->script);

            core_memory_copy(new_buffer + offset, bucket, sizeof(*bucket));
            offset += sizeof(*bucket);

            thorium_actor_send_reply_buffer(actor, ACTION_SPAWN_MANY, offset, new_buffer);

        } else {
            thorium_actor_send_reply_int(actor, ACTION_SPAWN, concrete_actor->script);
        }

    } else if (tag == ACTION_SPAWN_REPLY
                    || tag == ACTION_SPAWN_MANY_REPLY) {

        index = source;
        stores = core_map_get(&concrete_actor->spawner_children, &index);

        if (tag == ACTION_SPAWN_REPLY) {
            store = *(int *)buffer;
            core_vector_push_back(stores, &store);

        } else if (tag == ACTION_SPAWN_MANY_REPLY) {
            core_vector_init(&received_actors, sizeof(int));
            core_vector_set_memory_pool(&received_actors, ephemeral_memory);

            core_vector_unpack(&received_actors, buffer);
            core_vector_push_back_vector(stores, &received_actors);
            core_vector_destroy(&received_actors);
        }

        bucket = core_map_get(&concrete_actor->spawner_child_count, &index);

#ifdef CORE_MANAGER_DEBUG
        printf("DEBUG manager %d receives %d from spawner %d, now %d/%d\n",
                        thorium_actor_name(actor), store, source,
                        (int)core_vector_size(stores), *bucket);
#endif

        /*
         * There are still some actors to spawn.
         */
        if (core_vector_size(stores) >= *bucket) {

            concrete_actor->ready_spawners++;

            printf("DEBUG manager %d says that spawner %d is ready, %d/%d (spawned %d actors)\n",
                        thorium_actor_name(actor), source,
                        concrete_actor->ready_spawners, concrete_actor->spawners,
                        (int)core_vector_size(stores));

            if (concrete_actor->ready_spawners == concrete_actor->spawners) {

                printf("DEBUG manager %d says that all spawners are ready\n",
                        thorium_actor_name(actor));

                core_vector_init(&all_stores, sizeof(int));

                /*
                 * Use a memory pool to avoid memory fragmentation.
                 */
                core_vector_set_memory_pool(&all_stores, ephemeral_memory);

                core_vector_iterator_init(&iterator, &concrete_actor->indices);

                while (core_vector_iterator_has_next(&iterator)) {
                    core_vector_iterator_next(&iterator, (void **)&bucket);

                    index = *bucket;

                    stores = (struct core_vector *)core_map_get(&concrete_actor->spawner_children,
                                    &index);

                    core_vector_push_back_vector(&all_stores, stores);
                }

                core_vector_iterator_destroy(&iterator);

                new_count = core_vector_pack_size(&all_stores);
                new_buffer = thorium_actor_allocate(actor, new_count);
                core_vector_pack(&all_stores, new_buffer);

                /*
                 * Save the list of actors for later
                 */
                core_vector_push_back_vector(&concrete_actor->children,
                                &all_stores);

                core_vector_destroy(&all_stores);

                thorium_message_init(&new_message, ACTION_START_REPLY, new_count, new_buffer);
                thorium_actor_send_to_supervisor(actor, &new_message);

                thorium_message_destroy(&new_message);
            }
        } else {

            CORE_DEBUGGER_ASSERT(tag == ACTION_SPAWN_REPLY);

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

void core_manager_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message)
{

    struct core_manager *concrete_actor;
    int i;
    int size;
    int child;

    printf("%s/%d dies\n",
                    thorium_actor_script_name(actor),
                    thorium_actor_name(actor));

    concrete_actor = (struct core_manager *)thorium_actor_concrete_actor(actor);
    thorium_actor_ask_to_stop(actor, message);

    /*
     * Stop children too
     */

    size = core_vector_size(&concrete_actor->children);

    for (i = 0; i < size; i++) {

        child = core_vector_at_as_int(&concrete_actor->children, i);

        thorium_actor_send_empty(actor, child, ACTION_ASK_TO_STOP);
    }
}
