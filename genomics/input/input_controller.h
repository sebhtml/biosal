
#ifndef BIOSAL_INPUT_CONTROLLER_H
#define BIOSAL_INPUT_CONTROLLER_H

#include <engine/thorium/actor.h>

#include <core/structures/vector.h>
#include <core/structures/map.h>
#include <core/structures/queue.h>

#include <core/system/timer.h>

#include <genomics/data/dna_codec.h>


#define SCRIPT_INPUT_CONTROLLER 0x985607aa

struct biosal_input_controller {
    struct core_vector consumers;

    struct biosal_dna_codec codec;
    struct core_vector files;

    struct core_vector counting_streams;
    struct core_vector reading_streams;

    struct core_map mega_blocks;
    struct core_vector mega_block_vector;
    struct core_map assigned_blocks;

    struct core_vector spawners;
    int partitioner;

    int opened_streams;
    int counted;
    struct core_vector counts;
    struct core_vector partition_commands;
    struct core_vector stream_consumers;
    struct core_queue unprepared_spawners;
    int state;
    int block_size;
    int spawner;

    int ready_consumers;
    int filled_consumers;
    int ready_spawners;
    struct core_vector stores_per_spawner;
    int stores_per_worker_per_spawner;

    struct core_timer input_timer;
    struct core_timer counting_timer;
    struct core_timer distribution_timer;

    struct core_vector consumer_active_requests;
};

#define ACTION_INPUT_DISTRIBUTE 0x00003cbe
#define ACTION_INPUT_DISTRIBUTE_REPLY 0x00001ab0


#define ACTION_ADD_FILE 0x00007deb
#define ACTION_ADD_FILE_REPLY 0x00007036

#define ACTION_SET_BLOCK_SIZE 0x0000749e
#define ACTION_SET_BLOCK_SIZE_REPLY 0x00001461

#define ACTION_INPUT_SPAWN 0x00000a5d

#define ACTION_INPUT_CONTROLLER_CREATE_STORES 0x0000285f
#define ACTION_INPUT_CONTROLLER_CREATE_STORES_REPLY 0x000048ed
#define ACTION_INPUT_CONTROLLER_SPAWN_READING_STREAMS 0x00005341
#define ACTION_INPUT_CONTROLLER_PREPARE_SPAWNERS 0x00004a85
#define ACTION_INPUT_CONTROLLER_CREATE_PARTITION 0x00005b68

extern struct thorium_script biosal_input_controller_script;

#endif
