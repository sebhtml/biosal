
#include "sequence_partitioner.h"

#include "partition_command.h"

#include <core/structures/vector_iterator.h>

#include <core/helpers/message_helper.h>

#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

/*
#define BSAL_SEQUENCE_PARTITIONER_DEBUG

*/

struct bsal_script bsal_sequence_partitioner_script = {
    .identifier = BSAL_SEQUENCE_PARTITIONER_SCRIPT,
    .init = bsal_sequence_partitioner_init,
    .destroy = bsal_sequence_partitioner_destroy,
    .receive = bsal_sequence_partitioner_receive,
    .size = sizeof(struct bsal_sequence_partitioner),
    .name = "sequence_partitioner"
};

void bsal_sequence_partitioner_init(struct bsal_actor *actor)
{
    struct bsal_sequence_partitioner *concrete_actor;

    concrete_actor = (struct bsal_sequence_partitioner *)bsal_actor_concrete_actor(actor);

    bsal_vector_init(&concrete_actor->stream_entries, sizeof(uint64_t));
    bsal_vector_init(&concrete_actor->stream_positions, sizeof(uint64_t));
    bsal_vector_init(&concrete_actor->stream_global_positions, sizeof(uint64_t));
    bsal_vector_init(&concrete_actor->store_entries, sizeof(uint64_t));

    bsal_queue_init(&concrete_actor->available_commands, sizeof(struct bsal_partition_command));
    bsal_map_init(&concrete_actor->active_commands, sizeof(int),
                    sizeof(struct bsal_partition_command));

    concrete_actor->store_count = -1;
    concrete_actor->block_size = -1;

    concrete_actor->command_number = 0;

    concrete_actor->last_progress = -1;
}

void bsal_sequence_partitioner_destroy(struct bsal_actor *actor)
{
    struct bsal_sequence_partitioner *concrete_actor;

    concrete_actor = (struct bsal_sequence_partitioner *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&concrete_actor->stream_entries);
    bsal_vector_destroy(&concrete_actor->stream_positions);
    bsal_vector_destroy(&concrete_actor->stream_global_positions);
    bsal_vector_destroy(&concrete_actor->store_entries);

    bsal_queue_destroy(&concrete_actor->available_commands);
    bsal_map_destroy(&concrete_actor->active_commands);
}

void bsal_sequence_partitioner_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int count;
    void *buffer;
    int bytes;
    struct bsal_sequence_partitioner *concrete_actor;
    struct bsal_partition_command command;
    struct bsal_message response;
    int command_number;
    struct bsal_partition_command *active_command;
    int stream_index;
    struct bsal_partition_command *command_bucket;
    int i;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(actor);
    bsal_message_helper_get_all(message, &tag, &count, &buffer, &source);

    concrete_actor = (struct bsal_sequence_partitioner *)bsal_actor_concrete_actor(actor);

    if (tag == BSAL_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE) {
        bsal_message_helper_unpack_int(message, 0, &concrete_actor->block_size);
        bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_PARTITIONER_SET_BLOCK_SIZE_REPLY);

        bsal_sequence_partitioner_verify(actor);
/*
        printf("DEBUG bsal_sequence_partitioner_receive received block size\n");
        */
    } else if (tag == BSAL_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR) {

            /*
        printf("DEBUG bsal_sequence_partitioner_receive unpacking vector, %d bytes\n",
                        count);
                        */

        bsal_vector_init(&concrete_actor->stream_entries, 0);
        bsal_vector_unpack(&concrete_actor->stream_entries, buffer);

        /*
        printf("DEBUG after unpack\n");
        */

        bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_PARTITIONER_SET_ENTRY_VECTOR_REPLY);

        /*
        printf("DEBUG bsal_sequence_partitioner_receive received received entry vector\n");
        */
        bsal_sequence_partitioner_verify(actor);

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->store_count);
        bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_PARTITIONER_SET_ACTOR_COUNT_REPLY);

        bsal_sequence_partitioner_verify(actor);
        /*
        printf("DEBUG bsal_sequence_partitioner_receive received received store count\n");
        */

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_GET_COMMAND) {

        if (bsal_queue_dequeue(&concrete_actor->available_commands, &command)) {

            bytes = bsal_partition_command_pack_size(&command);

            /*
            printf("DEBUG partitioner has command, packing %d bytes!\n", bytes);
            */

            buffer = bsal_memory_pool_allocate(ephemeral_memory, bytes);
            bsal_partition_command_pack(&command, buffer);

            bsal_message_init(&response, BSAL_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY,
                            bytes, buffer);
            bsal_actor_send_reply(actor, &response);

            /* store the active command
             */
            command_number = bsal_partition_command_name(&command);
            command_bucket = (struct bsal_partition_command *)bsal_map_add(&concrete_actor->active_commands,
                            &command_number);
            *command_bucket = command;

            bsal_memory_pool_free(ephemeral_memory, buffer);

            /* there may be other command available too !
             */
        }

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_GET_COMMAND_REPLY_REPLY) {
        /*
         * take the name of the command, find it in the active
         * command, generate a new command, and send BSAL_SEQUENCE_PARTITIONER_COMMAND_IS_READY
         * as a reply
         */

        bsal_message_helper_unpack_int(message, 0, &command_number);

        active_command = bsal_map_get(&concrete_actor->active_commands,
                        &command_number);

        if (active_command == NULL) {
            return;
        }

        stream_index = bsal_partition_command_stream_index(active_command);
        active_command = NULL;
        bsal_map_delete(&concrete_actor->active_commands,
                        &command_number);

        bsal_sequence_partitioner_generate_command(actor, stream_index);

        if (bsal_map_size(&concrete_actor->active_commands) == 0
                        && bsal_queue_size(&concrete_actor->available_commands) == 0) {

            bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_PARTITIONER_FINISHED);
        }

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP
                    && source == bsal_actor_supervisor(actor)) {

#ifdef BSAL_SEQUENCE_PARTITIONER_DEBUG
        printf("DEBUG bsal_sequence_partitioner_receive BSAL_ACTOR_ASK_TO_STOP\n");
#endif

        bsal_actor_send_to_self_empty(actor,
                        BSAL_ACTOR_STOP);

    } else if (tag == BSAL_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS_REPLY) {
        /* generate commands
         */
        for (i = 0; i < bsal_vector_size(&concrete_actor->stream_entries); i++) {

            bsal_sequence_partitioner_generate_command(actor, i);
        }
    }
}

void bsal_sequence_partitioner_verify(struct bsal_actor *actor)
{
    struct bsal_sequence_partitioner *concrete_actor;
    int i;
    int64_t entries;
    uint64_t position;
    uint64_t stream_entries;
    int bytes;
    void *buffer;
    struct bsal_message message;
    int64_t remaining;
    int remainder;
    uint64_t *bucket_for_store_count;
    struct bsal_vector_iterator iterator;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(actor);
    concrete_actor = (struct bsal_sequence_partitioner *)bsal_actor_concrete_actor(actor);

    /*
     * check if parameters are
     * initialized
     */
    if (concrete_actor->block_size == -1) {
        return;
    }

    if (concrete_actor->store_count == -1) {
        return;
    }

    if (bsal_vector_size(&concrete_actor->stream_entries) == 0) {
        return;
    }

    /* at this point, all parameters are ready.
     * prepare <stream_entries.size> commands
     */

    position = 0;

    entries = 0;

    /*
    printf("DEBUG generating initial positions\n");
    */
    /* generate stream positions, stream global positions, and total
     */
    for (i = 0; i < bsal_vector_size(&concrete_actor->stream_entries); i++) {

        bsal_vector_push_back(&concrete_actor->stream_positions, &position);

        bsal_vector_push_back(&concrete_actor->stream_global_positions, &entries);

        stream_entries = *(uint64_t *)bsal_vector_at(&concrete_actor->stream_entries, i);

#ifdef BSAL_SEQUENCE_PARTITIONER_DEBUG
        printf("DEBUG stream_entries %i %" PRIu64 "\n",
                        i, stream_entries);
#endif

        entries += stream_entries;
    }

    concrete_actor->total = entries;

    /* compute the number of entries for each store
     */

    entries = concrete_actor->total / concrete_actor->store_count;

    /* make sure that this is a multiple of block size
     * examples:
     * total= 20000
     * store_count= 2
     * block_size= 8192
     * 20000 / 2 = 10000
     * 10000 % 8192 = 1808
     * difference = 8192 - 1808 = 6384
     * 10000 + 6384 = 16384
     */

    if (entries % concrete_actor->block_size != 0) {
        remainder = entries % concrete_actor->block_size;
        entries -= remainder;
    }

    /* make sure that at most one store has less
     * than block size
     */
    if (entries < concrete_actor->block_size) {
        entries = concrete_actor->block_size;
    }

#ifdef BSAL_SEQUENCE_PARTITIONER_DEBUG
    printf("DEBUG93 entries for stores %d\n", (int)entries);
#endif

    remaining = concrete_actor->total;

    if (remaining <= entries) {
        entries = remaining;
    }

    /* example: 10000, block_size 4096,  3 stores
     *
     * total entries remaining
     * 10000 4096    5904
     * 10000 4096    1808
     * 10000 1808    0
     */
    for (i = 0; i < concrete_actor->store_count; i++) {
        bsal_vector_push_back(&concrete_actor->store_entries, &entries);

        remaining -= entries;

        if (remaining < entries) {
            entries = remaining;
        }

    }

    bsal_vector_iterator_init(&iterator, &concrete_actor->store_entries);

    while (bsal_vector_iterator_has_next(&iterator)) {
        bsal_vector_iterator_next(&iterator, (void **)&bucket_for_store_count);

        if (remaining >= concrete_actor->block_size) {
            *bucket_for_store_count += concrete_actor->block_size;
            remaining -= concrete_actor->block_size;
        } else if (remaining == 0) {
            break;
        } else {
            /* between 1 and block_size - 1 inclusively
             */
            *bucket_for_store_count += remaining;
            remaining = 0;
        }
    }

    bsal_vector_iterator_destroy(&iterator);

#ifdef BSAL_SEQUENCE_PARTITIONER_DEBUG
    printf("DEBUG bsal_sequence_partitioner_verify sending store counts\n");
#endif

    bytes = bsal_vector_pack_size(&concrete_actor->store_entries);
    buffer = bsal_memory_pool_allocate(ephemeral_memory, bytes);
    bsal_vector_pack(&concrete_actor->store_entries, buffer);

    bsal_message_init(&message, BSAL_SEQUENCE_PARTITIONER_PROVIDE_STORE_ENTRY_COUNTS,
                    bytes, buffer);
    bsal_actor_send_reply(actor, &message);

    bsal_memory_pool_free(ephemeral_memory, buffer);
}

uint64_t bsal_sequence_partitioner_get_index_in_store(uint64_t index, int block_size, int store_count)
{
    /*
       irb(main):014:0> (1000000 / 8192) / 72 * 81 = 8192
     */

    return ( ( index / ((uint64_t)block_size) ) / (uint64_t)store_count ) * (uint64_t)block_size +
            index % block_size;
}

int bsal_sequence_partitioner_get_store(uint64_t index, int block_size, int store_count)
{
    /*
     * idea:
     *
     * first          ... last                 ........ store
     *
     * 0 * block_size ... 1 * block_size - 1   ........ store 0
     * 1 * block_size ... 2 * block_size - 1   ........ store 1
     * 2 * block_size ... 3 * block_size - 1   ........ store 2
     * 3 * block_size ... 3 * block_size - 1   ........ store 3
     * ...
     *
     * pattern:
     *
     * x * block_size ... (x + 1) * block_size - 1   ........ store (x % store_count)
     *
     *
     * Given an index i and block_size b, we have:
     *
     * (x * b) <= i <= ( (x + 1) * b - 1 )
     *
     * (x * b) <= i
     * x <= i / b (Equation 1.)
     *
     * i <= ( (x + 1) * b - 1 )
     * i <= (x + 1) * b - 1
     * i + 1 <= (x + 1) * b
     * ( i + 1 ) / b <= (x + 1)
     * ( i + 1 ) / b <= x + 1
     * ( i + 1 ) / b - 1 <= x (Equation 2.)
     *
     * So, dividing index by block_size gives x.
     */

    /* do the calculation in 64 bits to be sure.
     */

    if (store_count == 0) {
        return 0;
    }

    return ( index / (uint64_t) block_size) % ( (uint64_t) store_count);
}

void bsal_sequence_partitioner_generate_command(struct bsal_actor *actor, int stream_index)
{
    uint64_t *bucket_for_stream_position;
    uint64_t *bucket_for_global_position;
    struct bsal_partition_command command;

    int store_index;
    uint64_t stream_entries;
    uint64_t stream_first;
    uint64_t stream_last;
    int64_t available_in_stream;

    uint64_t store_first;
    uint64_t store_last;

    struct bsal_sequence_partitioner *concrete_actor;
    int64_t actual_block_size;
    int distance;

    uint64_t global_first;
    uint64_t global_last;
    uint64_t next_block_first;

    int command_name;

    int blocks;
    float progress;

    concrete_actor = (struct bsal_sequence_partitioner *)bsal_actor_concrete_actor(actor);

    /*
    printf("DEBUG bsal_sequence_partitioner_generate_command %d\n",
                    stream_index);
*/

    bucket_for_stream_position = (uint64_t *)bsal_vector_at(&concrete_actor->stream_positions, stream_index);
    bucket_for_global_position = (uint64_t *)bsal_vector_at(&concrete_actor->stream_global_positions,
                    stream_index);

    /*
    printf("DEBUG got buckets.\n");
*/

    /* compute feasible block size given the stream and the store.
     */
    global_first = *bucket_for_global_position;

    actual_block_size = concrete_actor->block_size;

    next_block_first = global_first +
            (concrete_actor->block_size - global_first % concrete_actor->block_size);

    distance = next_block_first - global_first;

    if (distance < actual_block_size) {
        actual_block_size = distance;
    }

    store_index = bsal_sequence_partitioner_get_store(global_first, concrete_actor->block_size,
                    concrete_actor->store_count);

    stream_entries = *(uint64_t *)bsal_vector_at(&concrete_actor->stream_entries, stream_index);

    stream_first = *bucket_for_stream_position;

    /* check out what is left in the stream
     */
    available_in_stream = stream_entries - *bucket_for_stream_position;

#ifdef BSAL_SEQUENCE_PARTITIONER_DEBUG
    printf("DEBUG stream_entries %" PRIu64 " !\n", stream_entries);
    printf("DEBUG bucket_for_stream_position %" PRIu64 " !\n", *bucket_for_stream_position);
    printf("DEBUG available_in_stream %" PRIu64 " !\n", available_in_stream);
#endif

    if (available_in_stream < actual_block_size) {
#ifdef BSAL_SEQUENCE_PARTITIONER_DEBUG
#endif
        actual_block_size = available_in_stream;
    }

    /* can't do that
     */
    if (actual_block_size == 0) {
#ifdef BSAL_SEQUENCE_PARTITIONER_DEBUG
        printf("DEBUG stream %d actual_block_size %d\n",
                        stream_index, actual_block_size);
#endif
        return;
    }

    stream_first = *bucket_for_stream_position;
    stream_last = stream_first + actual_block_size - 1;

    store_first = bsal_sequence_partitioner_get_index_in_store(global_first,
                    concrete_actor->block_size, concrete_actor->store_count);

    store_last = store_first + actual_block_size - 1;

    global_last = global_first + actual_block_size - 1;

    /*
    printf("DEBUG %" PRIu64 " goes in store %d\n",
                    global_first, store_index);
*/
    bsal_partition_command_init(&command, concrete_actor->command_number,
                    stream_index, stream_first, stream_last,
                    store_index, store_first, store_last,
                    global_first, global_last);

    concrete_actor->command_number++;

    bsal_queue_enqueue(&concrete_actor->available_commands,
                    &command);

#ifdef BSAL_SEQUENCE_PARTITIONER_DEBUG
    printf("DEBUG906 in partitioner:\n");
    bsal_partition_command_print(&command);
#endif

    /* update positions
     */

    *bucket_for_stream_position = stream_last + 1;
    *bucket_for_global_position = global_last + 1;

    /*
    printf("DEBUG command is ready\n");
    */

    /* emit a signal
     */
    bsal_actor_send_reply_empty(actor, BSAL_SEQUENCE_PARTITIONER_COMMAND_IS_READY);

    command_name = bsal_partition_command_name(&command);

    blocks = (int)(concrete_actor->total / concrete_actor->block_size);

    if (concrete_actor->total % concrete_actor->block_size != 0) {
        blocks++;
    }

    progress = (0.0 + command_name) / blocks;

    if (concrete_actor->last_progress < 0
                    || progress >= concrete_actor->last_progress + 0.04
                    || command_name == blocks - 1) {

        printf("partitioner/%d generated partition command # %d (total %" PRIu64 ", block_size %d, blocks %d, progress %.2f)\n",
                    bsal_actor_name(actor),
                    command_name,
                    concrete_actor->total, concrete_actor->block_size,
                    blocks, progress);

        concrete_actor->last_progress = progress;
    }

    bsal_partition_command_destroy(&command);

}
