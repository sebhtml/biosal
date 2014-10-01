
#ifndef BIOSAL_SEQUENCE_PARTITIONER_H
#define BIOSAL_SEQUENCE_PARTITIONER_H

#include <engine/thorium/actor.h>

#include <core/structures/vector.h>
#include <core/structures/map.h>
#include <core/structures/queue.h>

#define SCRIPT_SEQUENCE_PARTITIONER 0x3c0e30a4

/*
 * This actor generates I/O commands
 * given a vector of entries, an actor count
 * and a block size;
 */
struct biosal_sequence_partitioner {
    struct core_vector stream_entries;
    struct core_vector stream_positions;
    struct core_vector stream_global_positions;

    struct core_vector store_entries;

    uint64_t total;

    int block_size;
    int store_count;
    struct core_map active_commands;
    struct core_queue available_commands;

    int command_number;

    float last_progress;
};

#define ACTION_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE 0x000020ef
#define ACTION_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE_REPLY 0x00002d63
#define ACTION_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR 0x000009de
#define ACTION_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR_REPLY 0x000054b8
#define ACTION_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT 0x0000618d
#define ACTION_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT_REPLY 0x00001529

#define ACTION_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS 0x00002c39
#define ACTION_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS_REPLY 0x000056ed

#define ACTION_SEQUENCE_PARTITIONER_COMMAND_IS_READY 0x00002d74

#define ACTION_SEQUENCE_PARTITIONER_GET_COMMAND 0x00003662
#define ACTION_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY 0x0000116e
#define ACTION_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY_REPLY 0x00003444

#define ACTION_SEQUENCE_PARTITIONER_FINISHED 0x00005db5

extern struct thorium_script biosal_sequence_partitioner_script;

void biosal_sequence_partitioner_init(struct thorium_actor *actor);
void biosal_sequence_partitioner_destroy(struct thorium_actor *actor);
void biosal_sequence_partitioner_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_sequence_partitioner_verify(struct thorium_actor *actor);

int biosal_sequence_partitioner_get_store(uint64_t index, int block_size, int store_count);
uint64_t biosal_sequence_partitioner_get_index_in_store(uint64_t index, int block_size, int store_count);
void biosal_sequence_partitioner_generate_command(struct thorium_actor *actor, int stream_index);

#endif
