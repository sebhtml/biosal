
#ifndef BSAL_MANAGER_H
#define BSAL_MANAGER_H

#include <engine/actor.h>

#include <structures/dynamic_hash_table.h>
#include <structures/vector.h>

#define BSAL_MANAGER_SCRIPT 0xe41954c7

struct bsal_manager {
    struct bsal_dynamic_hash_table spawner_children;
    struct bsal_dynamic_hash_table spawner_child_count;
    int ready_spawners;
    int spawners;
    struct bsal_vector indices;
    int script;

    int actors_per_spawner;
};

#define BSAL_MANAGER_SET_SCRIPT 0x00007595
#define BSAL_MANAGER_SET_SCRIPT_REPLY 0x00007667
#define BSAL_MANAGER_SET_ACTORS_PER_SPAWNER 0x000044d9
#define BSAL_MANAGER_SET_ACTORS_PER_SPAWNER_REPLY 0x00005b4c

extern struct bsal_script bsal_manager_script;

void bsal_manager_init(struct bsal_actor *actor);
void bsal_manager_destroy(struct bsal_actor *actor);
void bsal_manager_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
