
#ifndef BSAL_INPUT_CONTROLLER_H
#define BSAL_INPUT_CONTROLLER_H

#include <engine/actor.h>
#include <structures/vector.h>
#include <structures/queue.h>

#define BSAL_INPUT_CONTROLLER_SCRIPT 0x985607aa

struct bsal_input_controller {
    struct bsal_vector files;
    struct bsal_vector streams;
    struct bsal_vector spawners;
    struct bsal_vector stores;
    int opened_streams;
    int counted;
    struct bsal_vector counts;
    struct bsal_vector stores_per_spawner;
    struct bsal_queue unprepared_spawners;
    int state;
};

#define BSAL_INPUT_DISTRIBUTE 0x00003cbe
#define BSAL_INPUT_STOP 0x00004107
#define BSAL_INPUT_CONTROLLER_START 0x000000d0
#define BSAL_INPUT_CONTROLLER_START_REPLY 0x00004e66
#define BSAL_ADD_FILE 0x00007deb
#define BSAL_ADD_FILE_REPLY 0x00007036
#define BSAL_INPUT_DISTRIBUTE_REPLY 0x00001ab0
#define BSAL_INPUT_SPAWN 0x00000a5d
#define BSAL_INPUT_CONTROLLER_CREATE_STORES 0x0000285f
#define BSAL_INPUT_CONTROLLER_CREATE_STORES_REPLY 0x000048ed
#define BSAL_INPUT_CONTROLLER_PREPARE_SPAWNERS 0x00004a85

extern struct bsal_script bsal_input_controller_script;

void bsal_input_controller_init(struct bsal_actor *actor);
void bsal_input_controller_destroy(struct bsal_actor *actor);
void bsal_input_controller_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_input_controller_create_stores(struct bsal_actor *actor, struct bsal_message *message);
void bsal_input_controller_get_node_name_reply(struct bsal_actor *actor, struct bsal_message *message);
void bsal_input_controller_get_node_worker_count_reply(struct bsal_actor *actor, struct bsal_message *message);
void bsal_input_controller_add_store(struct bsal_actor *actor, struct bsal_message *message);
void bsal_input_controller_prepare_spawners(struct bsal_actor *actor, struct bsal_message *message);

#endif
