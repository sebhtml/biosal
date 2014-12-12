
#include "coverage_distribution.h"

#include <genomics/helpers/command.h>

#include <core/helpers/vector_helper.h>
#include <engine/thorium/modules/message_helper.h>

#include <core/structures/map_iterator.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/string.h>

#include <core/system/memory.h>
#include <core/system/command.h>
#include <core/system/debugger.h>

#include <core/file_storage/directory.h>
#include <core/file_storage/output/buffered_file_writer.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <inttypes.h>

void biosal_coverage_distribution_init(struct thorium_actor *actor);
void biosal_coverage_distribution_destroy(struct thorium_actor *actor);
void biosal_coverage_distribution_receive(struct thorium_actor *actor, struct thorium_message *message);

void biosal_coverage_distribution_write_distribution(struct thorium_actor *self);
void biosal_coverage_distribution_ask_to_stop(struct thorium_actor *self, struct thorium_message *message);

struct thorium_script biosal_coverage_distribution_script = {
    .identifier = SCRIPT_COVERAGE_DISTRIBUTION,
    .name = "biosal_coverage_distribution",
    .init = biosal_coverage_distribution_init,
    .destroy = biosal_coverage_distribution_destroy,
    .receive = biosal_coverage_distribution_receive,
    .size = sizeof(struct biosal_coverage_distribution)
};

void biosal_coverage_distribution_init(struct thorium_actor *self)
{
    struct biosal_coverage_distribution *concrete_actor;

    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);

    core_map_init(&concrete_actor->distribution, sizeof(int), sizeof(uint64_t));

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
    thorium_actor_log(self, "DISTRIBUTION IS READY\n");
#endif
    concrete_actor->actual = 0;
    concrete_actor->expected = 0;

    thorium_actor_log(self, "%s/%d is ready\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self));
}

void biosal_coverage_distribution_destroy(struct thorium_actor *self)
{
    struct biosal_coverage_distribution *concrete_actor;

    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);

    core_map_destroy(&concrete_actor->distribution);
}

void biosal_coverage_distribution_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    struct core_map map;
    struct core_map_iterator iterator;
    int *coverage_from_message;
    uint64_t *count_from_message;
    uint64_t *frequency;
    int count;
    void *buffer;
    struct biosal_coverage_distribution *concrete_actor;
    int name;
    int source;
    struct core_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    name = thorium_actor_name(self);
    source = thorium_message_source(message);
    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);
    tag = thorium_message_action(message);
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_PUSH_DATA) {

        core_map_init(&map, 0, 0);
        core_map_set_memory_pool(&map, ephemeral_memory);
        core_map_unpack(&map, buffer);

        core_map_iterator_init(&iterator, &map);


        while (core_map_iterator_has_next(&iterator)) {

            core_map_iterator_next(&iterator, (void **)&coverage_from_message,
                            (void **)&count_from_message);

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
            thorium_actor_log(self, "DEBUG DATA %d %d\n", (int)*coverage_from_message, (int)*count_from_message);
#endif

            frequency = core_map_get(&concrete_actor->distribution, coverage_from_message);

            if (frequency == NULL) {

                frequency = core_map_add(&concrete_actor->distribution, coverage_from_message);

                (*frequency) = 0;
            }

            (*frequency) += (*count_from_message);
        }

        core_map_iterator_destroy(&iterator);

        thorium_actor_send_reply_empty(self, ACTION_PUSH_DATA_REPLY);

        concrete_actor->actual++;

        thorium_actor_log(self, "distribution/%d receives coverage data from producer/%d, %d entries / %d bytes %d/%d\n",
                        name, source, (int)core_map_size(&map), count,
                        concrete_actor->actual, concrete_actor->expected);

        if (concrete_actor->expected != 0 && concrete_actor->expected == concrete_actor->actual) {

            thorium_actor_log(self, "received everything %d/%d\n", concrete_actor->actual, concrete_actor->expected);

            biosal_coverage_distribution_write_distribution(self);

            thorium_actor_send_empty(self, concrete_actor->source,
                            ACTION_NOTIFY);
        }

        core_map_destroy(&map);

    } else if (tag == ACTION_ASK_TO_STOP) {

        biosal_coverage_distribution_ask_to_stop(self, message);

    } else if (tag == ACTION_SET_EXPECTED_MESSAGE_COUNT) {

        concrete_actor->source = source;
        thorium_message_unpack_int(message, 0, &concrete_actor->expected);

        thorium_actor_log(self, "distribution %d expects %d messages\n",
                        thorium_actor_name(self),
                        concrete_actor->expected);

        thorium_actor_send_reply_empty(self, ACTION_SET_EXPECTED_MESSAGE_COUNT_REPLY);
    }
}

void biosal_coverage_distribution_write_distribution(struct thorium_actor *self)
{
    struct core_map_iterator iterator;
    int *coverage;
    uint64_t *canonical_frequency;
    uint64_t frequency;
    struct biosal_coverage_distribution *concrete_actor;
    struct core_vector coverage_values;
    struct core_vector_iterator vector_iterator;
    struct core_buffered_file_writer descriptor;
    struct core_buffered_file_writer descriptor_canonical;
    struct core_string file_name;
    struct core_string canonical_file_name;
    int argc;
    char **argv;
    int name;
    char *directory_name;

    name = thorium_actor_name(self);
    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);

    directory_name = biosal_command_get_output_directory(argc, argv);

    /* Create the directory if it does not exist
     */

    if (!core_directory_verify_existence(directory_name)) {

        core_directory_create(directory_name);
    }

    core_string_init(&file_name, "");
    core_string_append(&file_name, directory_name);
    core_string_append(&file_name, "/");
    core_string_append(&file_name, BIOSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE);

    core_string_init(&canonical_file_name, "");
    core_string_append(&canonical_file_name, directory_name);
    core_string_append(&canonical_file_name, "/");
    core_string_append(&canonical_file_name, BIOSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE_CANONICAL);

    core_buffered_file_writer_init(&descriptor, core_string_get(&file_name));
    core_buffered_file_writer_init(&descriptor_canonical, core_string_get(&canonical_file_name));

    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);

    core_vector_init(&coverage_values, sizeof(int));
    core_map_iterator_init(&iterator, &concrete_actor->distribution);

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
    thorium_actor_log(self, "map size %d\n", (int)core_map_size(&concrete_actor->distribution));
#endif

    while (core_map_iterator_has_next(&iterator)) {
        core_map_iterator_next(&iterator, (void **)&coverage, (void **)&canonical_frequency);

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
        thorium_actor_log(self, "DEBUG COVERAGE %d FREQUENCY %" PRIu64 "\n", *coverage, *frequency);
#endif

        core_vector_push_back(&coverage_values, coverage);
    }

    core_map_iterator_destroy(&iterator);

    core_vector_sort_int(&coverage_values);

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
    thorium_actor_log(self, "after sort ");
    core_vector_print_int(&coverage_values);
    thorium_actor_log(self, "\n");
#endif

    core_vector_iterator_init(&vector_iterator, &coverage_values);

#if 0
    core_buffered_file_writer_printf(&descriptor_canonical, "Coverage\tFrequency\n");
#endif

    core_buffered_file_writer_printf(&descriptor, "Coverage\tFrequency\n");
#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
#endif

    while (core_vector_iterator_has_next(&vector_iterator)) {

        core_vector_iterator_next(&vector_iterator, (void **)&coverage);

        canonical_frequency = (uint64_t *)core_map_get(&concrete_actor->distribution, coverage);

        frequency = 2 * *canonical_frequency;

        core_buffered_file_writer_printf(&descriptor_canonical, "%d %" PRIu64 "\n",
                        *coverage,
                        *canonical_frequency);

        core_buffered_file_writer_printf(&descriptor, "%d\t%" PRIu64 "\n",
                        *coverage,
                        frequency);
    }

    core_vector_destroy(&coverage_values);
    core_vector_iterator_destroy(&vector_iterator);

    thorium_actor_log(self, "distribution %d wrote %s\n", name, core_string_get(&file_name));
    thorium_actor_log(self, "distribution %d wrote %s\n", name, core_string_get(&canonical_file_name));

    core_buffered_file_writer_destroy(&descriptor);
    core_buffered_file_writer_destroy(&descriptor_canonical);

    core_string_destroy(&file_name);
    core_string_destroy(&canonical_file_name);
}

void biosal_coverage_distribution_ask_to_stop(struct thorium_actor *self, struct thorium_message *message)
{
    struct core_map_iterator iterator;
    struct core_vector coverage_values;
    struct biosal_coverage_distribution *concrete_actor;

    uint64_t *frequency;
    int *coverage;

    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);
    core_map_iterator_init(&iterator, &concrete_actor->distribution);

    core_vector_init(&coverage_values, sizeof(int));

    while (core_map_iterator_has_next(&iterator)) {

#if 0
        thorium_actor_log(self, "DEBUG EMIT iterator\n");
#endif
        core_map_iterator_next(&iterator, (void **)&coverage,
                        (void **)&frequency);

#ifdef CORE_DEBUGGER_ASSERT_ENABLED
        if (coverage == NULL) {
            thorium_actor_log(self, "DEBUG map has %d buckets\n", (int)core_map_size(&concrete_actor->distribution));
        }
#endif
        CORE_DEBUGGER_ASSERT(coverage != NULL);

        core_vector_push_back(&coverage_values, coverage);
    }

    core_vector_sort_int(&coverage_values);

    core_map_iterator_destroy(&iterator);

    core_vector_destroy(&coverage_values);

    thorium_actor_ask_to_stop(self, message);
}

