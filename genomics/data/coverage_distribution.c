
#include "coverage_distribution.h"

#include <core/helpers/vector_helper.h>
#include <core/helpers/message_helper.h>

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

    biosal_map_init(&concrete_actor->distribution, sizeof(int), sizeof(uint64_t));

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
    printf("DISTRIBUTION IS READY\n");
#endif
    concrete_actor->actual = 0;
    concrete_actor->expected = 0;
}

void biosal_coverage_distribution_destroy(struct thorium_actor *self)
{
    struct biosal_coverage_distribution *concrete_actor;

    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);

    biosal_map_destroy(&concrete_actor->distribution);
}

void biosal_coverage_distribution_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int tag;
    struct biosal_map map;
    struct biosal_map_iterator iterator;
    int *coverage_from_message;
    uint64_t *count_from_message;
    uint64_t *frequency;
    int count;
    void *buffer;
    struct biosal_coverage_distribution *concrete_actor;
    int name;
    int source;
    struct biosal_memory_pool *ephemeral_memory;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self);
    name = thorium_actor_name(self);
    source = thorium_message_source(message);
    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);
    tag = thorium_message_action(message);
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_PUSH_DATA) {

        biosal_map_init(&map, 0, 0);
        biosal_map_set_memory_pool(&map, ephemeral_memory);
        biosal_map_unpack(&map, buffer);

        biosal_map_iterator_init(&iterator, &map);


        while (biosal_map_iterator_has_next(&iterator)) {

            biosal_map_iterator_next(&iterator, (void **)&coverage_from_message,
                            (void **)&count_from_message);

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
            printf("DEBUG DATA %d %d\n", (int)*coverage_from_message, (int)*count_from_message);
#endif

            frequency = biosal_map_get(&concrete_actor->distribution, coverage_from_message);

            if (frequency == NULL) {

                frequency = biosal_map_add(&concrete_actor->distribution, coverage_from_message);

                (*frequency) = 0;
            }

            (*frequency) += (*count_from_message);
        }

        biosal_map_iterator_destroy(&iterator);

        thorium_actor_send_reply_empty(self, ACTION_PUSH_DATA_REPLY);

        concrete_actor->actual++;

        printf("distribution/%d receives coverage data from producer/%d, %d entries / %d bytes %d/%d\n",
                        name, source, (int)biosal_map_size(&map), count,
                        concrete_actor->actual, concrete_actor->expected);

        if (concrete_actor->expected != 0 && concrete_actor->expected == concrete_actor->actual) {

            printf("received everything %d/%d\n", concrete_actor->actual, concrete_actor->expected);

            biosal_coverage_distribution_write_distribution(self);

            thorium_actor_send_empty(self, concrete_actor->source,
                            ACTION_NOTIFY);
        }

        biosal_map_destroy(&map);

    } else if (tag == ACTION_ASK_TO_STOP) {

        biosal_coverage_distribution_ask_to_stop(self, message);

    } else if (tag == ACTION_SET_EXPECTED_MESSAGE_COUNT) {

        concrete_actor->source = source;
        thorium_message_unpack_int(message, 0, &concrete_actor->expected);

        printf("distribution %d expects %d messages\n",
                        thorium_actor_name(self),
                        concrete_actor->expected);

        thorium_actor_send_reply_empty(self, ACTION_SET_EXPECTED_MESSAGE_COUNT_REPLY);
    }
}

void biosal_coverage_distribution_write_distribution(struct thorium_actor *self)
{
    struct biosal_map_iterator iterator;
    int *coverage;
    uint64_t *canonical_frequency;
    uint64_t frequency;
    struct biosal_coverage_distribution *concrete_actor;
    struct biosal_vector coverage_values;
    struct biosal_vector_iterator vector_iterator;
    struct biosal_buffered_file_writer descriptor;
    struct biosal_buffered_file_writer descriptor_canonical;
    struct biosal_string file_name;
    struct biosal_string canonical_file_name;
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

    if (!biosal_directory_verify_existence(directory_name)) {

        biosal_directory_create(directory_name);
    }

    biosal_string_init(&file_name, "");
    biosal_string_append(&file_name, directory_name);
    biosal_string_append(&file_name, "/");
    biosal_string_append(&file_name, BIOSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE);

    biosal_string_init(&canonical_file_name, "");
    biosal_string_append(&canonical_file_name, directory_name);
    biosal_string_append(&canonical_file_name, "/");
    biosal_string_append(&canonical_file_name, BIOSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE_CANONICAL);

    biosal_buffered_file_writer_init(&descriptor, biosal_string_get(&file_name));
    biosal_buffered_file_writer_init(&descriptor_canonical, biosal_string_get(&canonical_file_name));

    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);

    biosal_vector_init(&coverage_values, sizeof(int));
    biosal_map_iterator_init(&iterator, &concrete_actor->distribution);

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
    printf("map size %d\n", (int)biosal_map_size(&concrete_actor->distribution));
#endif

    while (biosal_map_iterator_has_next(&iterator)) {
        biosal_map_iterator_next(&iterator, (void **)&coverage, (void **)&canonical_frequency);

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
        printf("DEBUG COVERAGE %d FREQUENCY %" PRIu64 "\n", *coverage, *frequency);
#endif

        biosal_vector_push_back(&coverage_values, coverage);
    }

    biosal_map_iterator_destroy(&iterator);

    biosal_vector_sort_int(&coverage_values);

#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
    printf("after sort ");
    biosal_vector_print_int(&coverage_values);
    printf("\n");
#endif

    biosal_vector_iterator_init(&vector_iterator, &coverage_values);

#if 0
    biosal_buffered_file_writer_printf(&descriptor_canonical, "Coverage\tFrequency\n");
#endif

    biosal_buffered_file_writer_printf(&descriptor, "Coverage\tFrequency\n");
#ifdef BIOSAL_COVERAGE_DISTRIBUTION_DEBUG
#endif

    while (biosal_vector_iterator_has_next(&vector_iterator)) {

        biosal_vector_iterator_next(&vector_iterator, (void **)&coverage);

        canonical_frequency = (uint64_t *)biosal_map_get(&concrete_actor->distribution, coverage);

        frequency = 2 * *canonical_frequency;

        biosal_buffered_file_writer_printf(&descriptor_canonical, "%d %" PRIu64 "\n",
                        *coverage,
                        *canonical_frequency);

        biosal_buffered_file_writer_printf(&descriptor, "%d\t%" PRIu64 "\n",
                        *coverage,
                        frequency);
    }

    biosal_vector_destroy(&coverage_values);
    biosal_vector_iterator_destroy(&vector_iterator);

    printf("distribution %d wrote %s\n", name, biosal_string_get(&file_name));
    printf("distribution %d wrote %s\n", name, biosal_string_get(&canonical_file_name));

    biosal_buffered_file_writer_destroy(&descriptor);
    biosal_buffered_file_writer_destroy(&descriptor_canonical);

    biosal_string_destroy(&file_name);
    biosal_string_destroy(&canonical_file_name);
}

void biosal_coverage_distribution_ask_to_stop(struct thorium_actor *self, struct thorium_message *message)
{
    struct biosal_map_iterator iterator;
    struct biosal_vector coverage_values;
    struct biosal_coverage_distribution *concrete_actor;

    uint64_t *frequency;
    int *coverage;

    concrete_actor = (struct biosal_coverage_distribution *)thorium_actor_concrete_actor(self);
    biosal_map_iterator_init(&iterator, &concrete_actor->distribution);

    biosal_vector_init(&coverage_values, sizeof(int));

    while (biosal_map_iterator_has_next(&iterator)) {

#if 0
        printf("DEBUG EMIT iterator\n");
#endif
        biosal_map_iterator_next(&iterator, (void **)&coverage,
                        (void **)&frequency);

#ifdef BIOSAL_DEBUGGER_ENABLE_ASSERT
        if (coverage == NULL) {
            printf("DEBUG map has %d buckets\n", (int)biosal_map_size(&concrete_actor->distribution));
        }
#endif
        BIOSAL_DEBUGGER_ASSERT(coverage != NULL);

        biosal_vector_push_back(&coverage_values, coverage);
    }

    biosal_vector_sort_int(&coverage_values);

    biosal_map_iterator_destroy(&iterator);

    biosal_vector_destroy(&coverage_values);

    thorium_actor_ask_to_stop(self, message);
}

