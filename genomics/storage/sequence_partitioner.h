
#ifndef BSAL_SEQUENCE_PARTITIONER_H
#define BSAL_SEQUENCE_PARTITIONER_H

#include <engine/thorium/actor.h>

#include <core/structures/vector.h>
#include <core/structures/dynamic_hash_table.h>
#include <core/structures/queue.h>

#define BSAL_SEQUENCE_PARTITIONER_SCRIPT 0x3c0e30a4

/*
 * This actor generates I/O commands
 * given a vector of entries, an actor count
 * and a block size;
 */
struct bsal_sequence_partitioner {
    struct bsal_vector stream_entries;
    struct bsal_vector stream_positions;
    struct bsal_vector stream_global_positions;

    struct bsal_vector store_entries;

    uint64_t total;

    int block_size;
    int store_count;
    struct bsal_map active_commands;
    struct bsal_queue available_commands;

    int command_number;
};

#define BSAL_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE 0x000020ef
#define BSAL_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE_REPLY 0x00002d63
#define BSAL_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR 0x000009de
#define BSAL_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR_REPLY 0x000054b8
#define BSAL_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT 0x0000618d
#define BSAL_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT_REPLY 0x00001529

#define BSAL_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS 0x00002c39
#define BSAL_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS_REPLY 0x000056ed

#define BSAL_SEQUENCE_PARTITIONER_COMMAND_IS_READY 0x00002d74

#define BSAL_SEQUENCE_PARTITIONER_GET_COMMAND 0x00003662
#define BSAL_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY 0x0000116e
#define BSAL_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY_REPLY 0x00003444

#define BSAL_SEQUENCE_PARTITIONER_FINISHED 0x00005db5

extern struct bsal_script bsal_sequence_partitioner_script;

void bsal_sequence_partitioner_init(struct bsal_actor *actor);
void bsal_sequence_partitioner_destroy(struct bsal_actor *actor);
void bsal_sequence_partitioner_receive(struct bsal_actor *actor, struct bsal_message *message);

void bsal_sequence_partitioner_verify(struct bsal_actor *actor);

int bsal_sequence_partitioner_get_store(uint64_t index, int block_size, int store_count);
uint64_t bsal_sequence_partitioner_get_index_in_store(uint64_t index, int block_size, int store_count);
void bsal_sequence_partitioner_generate_command(struct bsal_actor *actor, int stream_index);

#endif
