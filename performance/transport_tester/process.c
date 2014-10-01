
#include "process.h"

#include <core/helpers/statistics.h>
#include <core/hash/hash.h>
#include <core/system/command.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

#define SEED 0x92a96a40

#define MIN_BUFFER_SIZE_OPTION "-minimum-buffer-size"
#define MAX_BUFFER_SIZE_OPTION "-maximum-buffer-size"
#define EVENT_COUNT_OPTION "-event-count"
#define CONCURRENT_EVENT_COUNT_OPTION "-concurrent-event-count"

struct thorium_script process_script = {
    .identifier = SCRIPT_TRANSPORT_PROCESS,
    .init = process_init,
    .destroy = process_destroy,
    .receive = process_receive,
    .size = sizeof(struct process),
    .name = "process"
};

void process_init(struct thorium_actor *self)
{
    struct process *concrete_self;
    int argc;
    char **argv;

    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);

    concrete_self = thorium_actor_concrete_actor(self);
    biosal_vector_init(&concrete_self->actors, sizeof(int));

    thorium_actor_add_action(self, ACTION_START, process_start);
    thorium_actor_add_action(self, ACTION_ASK_TO_STOP, process_stop);
    thorium_actor_add_action(self, ACTION_PING, process_ping);
    thorium_actor_add_action(self, ACTION_PING_REPLY, process_ping_reply);
    thorium_actor_add_action(self, ACTION_NOTIFY, process_notify);

    concrete_self->passed = 0;
    concrete_self->failed = 0;
    concrete_self->events = 0;

    concrete_self->minimum_buffer_size = 16;
    concrete_self->maximum_buffer_size = 512*1024;

    if (biosal_command_has_argument(argc, argv, MIN_BUFFER_SIZE_OPTION)) {
        concrete_self->maximum_buffer_size = biosal_command_get_argument_value_int(argc, argv, MIN_BUFFER_SIZE_OPTION);
    }

    if (biosal_command_has_argument(argc, argv, MAX_BUFFER_SIZE_OPTION)) {
        concrete_self->maximum_buffer_size = biosal_command_get_argument_value_int(argc, argv, MAX_BUFFER_SIZE_OPTION);
    }

    concrete_self->event_count = 100000;

    if (biosal_command_has_argument(argc, argv, EVENT_COUNT_OPTION)) {
        concrete_self->event_count = biosal_command_get_argument_value_int(argc, argv, EVENT_COUNT_OPTION);
    }

    concrete_self->concurrent_event_count = 8;

    if (biosal_command_has_argument(argc, argv, CONCURRENT_EVENT_COUNT_OPTION)) {
        concrete_self->concurrent_event_count = biosal_command_get_argument_value_int(argc, argv, CONCURRENT_EVENT_COUNT_OPTION);
    }

    concrete_self->active_messages = 0;

    printf("%s/%d using %s %d %s %d %s %d %s %d\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    MIN_BUFFER_SIZE_OPTION,
                    concrete_self->minimum_buffer_size,
                    MAX_BUFFER_SIZE_OPTION,
                    concrete_self->maximum_buffer_size,
                    EVENT_COUNT_OPTION,
                    concrete_self->event_count,
                    CONCURRENT_EVENT_COUNT_OPTION,
                    concrete_self->concurrent_event_count);
}

void process_destroy(struct thorium_actor *self)
{
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    biosal_vector_destroy(&concrete_self->actors);
}

void process_receive(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_take_action(self, message);
}

void process_ping(struct thorium_actor *self, struct thorium_message *message)
{
    int count;
    char *buffer;
    int buffer_size;
    uint64_t *bucket;
    uint64_t expected_checksum;
    uint64_t actual_checksum;
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);
    count = thorium_message_count(message);
    buffer_size = count - sizeof(expected_checksum);
    bucket = (uint64_t *)(buffer + buffer_size);
    expected_checksum = *bucket;
    actual_checksum = biosal_hash_data_uint64_t(buffer, buffer_size, SEED);

    if (expected_checksum != actual_checksum) {
        printf("TRANSPORT FAILED source: %d (%d) destination: %d (%d) tag: ACTION_PING count: %d"
                        " expected_checksum: %" PRIu64 " actual_checksum: %" PRIu64 "\n",
                        thorium_message_source(message),
                        thorium_message_source_node(message),
                        thorium_message_destination(message),
                        thorium_message_destination_node(message),
                        count,
                        expected_checksum, actual_checksum);

        ++concrete_self->failed;
    } else {
        ++concrete_self->passed;
    }

    thorium_actor_send_reply_empty(self, ACTION_PING_REPLY);
}

void process_start(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    biosal_vector_unpack(&concrete_self->actors, buffer);

    concrete_self->ready = 0;

    while (concrete_self->active_messages < concrete_self->concurrent_event_count) {
        process_send_ping(self);
    }
}

void process_stop(struct thorium_actor *self, struct thorium_message *message)
{
    struct process *concrete_self;
    int total;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);
    total = concrete_self->passed + concrete_self->failed;
    printf("%s/%d PASSED %d/%d, FAILED %d/%d\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    concrete_self->passed, total,
                    concrete_self->failed, total);

    thorium_actor_send_to_self_empty(self, ACTION_STOP);
}

void process_ping_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct process *concrete_self;
    int destination;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);

    ++concrete_self->events;
    --concrete_self->active_messages;

    if (concrete_self->events % 1000 == 0) {
        printf("PROGRESS %d/%d\n", concrete_self->events, concrete_self->event_count);
    }

    if (concrete_self->events < concrete_self->event_count) {
        process_send_ping(self);
    } else {
        destination = biosal_vector_at_as_int(&concrete_self->actors, 0);

        thorium_actor_send_empty(self, destination, ACTION_NOTIFY);
    }
}

void process_send_ping(struct thorium_actor *self)
{
    struct process *concrete_self;
    int destination;
    int size;
    int index;
    int buffer_size;
    int range;
    uint64_t checksum;
    int count;
    char *buffer;
    struct biosal_memory_pool *ephemeral_memory;
    int i;
    uint64_t *bucket;

    concrete_self = thorium_actor_concrete_actor(self);
    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);

    range = concrete_self->maximum_buffer_size - concrete_self->minimum_buffer_size;

    buffer_size = rand() % range;
    buffer_size += concrete_self->minimum_buffer_size;
    count = buffer_size + sizeof(checksum);

    buffer = thorium_actor_allocate(self, count);

    /*
     * Generate content;
     */
    for (i = 0; i < buffer_size; ++i) {
        buffer[i] = i % 256;
    }

    checksum = biosal_hash_data_uint64_t(buffer, buffer_size, SEED);
    bucket = (uint64_t *)(buffer + buffer_size);
    *bucket = checksum;

    size = biosal_vector_size(&concrete_self->actors);
    index = rand() % size;

    destination = biosal_vector_at_as_int(&concrete_self->actors, index);

    thorium_actor_send_buffer(self, destination, ACTION_PING, count, buffer);
    ++concrete_self->active_messages;
}

void process_notify(struct thorium_actor *self, struct thorium_message *message)
{
    struct process *concrete_self;

    concrete_self = (struct process *)thorium_actor_concrete_actor(self);

    ++concrete_self->ready;

    if (concrete_self->ready == biosal_vector_size(&concrete_self->actors)) {
        thorium_actor_send_range_empty(self, &concrete_self->actors,
                        ACTION_ASK_TO_STOP);
    }
}
