
#ifndef BSAL_SEQUENCE_STORE_MANAGER_H
#define BSAL_SEQUENCE_STORE_MANAGER_H

#include <engine/actor.h>

#include <structures/dynamic_hash_table.h>
#include <structures/vector.h>

#define BSAL_SEQUENCE_STORE_MANAGER_SCRIPT 0xe41954c7

struct bsal_sequence_store_manager {
    struct bsal_dynamic_hash_table spawner_stores;
    struct bsal_dynamic_hash_table spawner_store_count;
    int ready_spawners;
    int spawners;
    struct bsal_vector indices;
};

extern struct bsal_script bsal_sequence_store_manager_script;

void bsal_sequence_store_manager_init(struct bsal_actor *actor);
void bsal_sequence_store_manager_destroy(struct bsal_actor *actor);
void bsal_sequence_store_manager_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
