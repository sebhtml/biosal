
#include "coverage_distribution.h"

#include <core/helpers/actor_helper.h>
#include <core/helpers/vector_helper.h>
#include <core/helpers/message_helper.h>

#include <core/structures/map_iterator.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/string.h>

#include <core/system/memory.h>
#include <core/system/debugger.h>
#include <core/system/directory.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <inttypes.h>

struct bsal_script bsal_coverage_distribution_script = {
    .name = BSAL_COVERAGE_DISTRIBUTION_SCRIPT,
    .init = bsal_coverage_distribution_init,
    .destroy = bsal_coverage_distribution_destroy,
    .receive = bsal_coverage_distribution_receive,
    .size = sizeof(struct bsal_coverage_distribution),
    .description = "coverage_distribution"
};

void bsal_coverage_distribution_init(struct bsal_actor *self)
{
    struct bsal_coverage_distribution *concrete_actor;

    concrete_actor = (struct bsal_coverage_distribution *)bsal_actor_concrete_actor(self);

    bsal_map_init(&concrete_actor->distribution, sizeof(int), sizeof(uint64_t));

#ifdef BSAL_COVERAGE_DISTRIBUTION_DEBUG
    printf("DISTRIBUTION IS READY\n");
#endif
    concrete_actor->actual = 0;
    concrete_actor->expected = 0;
}

void bsal_coverage_distribution_destroy(struct bsal_actor *self)
{
    struct bsal_coverage_distribution *concrete_actor;

    concrete_actor = (struct bsal_coverage_distribution *)bsal_actor_concrete_actor(self);

    bsal_map_destroy(&concrete_actor->distribution);
}

void bsal_coverage_distribution_receive(struct bsal_actor *self, struct bsal_message *message)
{
    int tag;
    struct bsal_map map;
    struct bsal_map_iterator iterator;
    int *coverage_from_message;
    uint64_t *count_from_message;
    uint64_t *frequency;
    int count;
    void *buffer;
    struct bsal_coverage_distribution *concrete_actor;
    int name;
    int source;
    struct bsal_memory_pool *ephemeral_memory;

    ephemeral_memory = bsal_actor_get_ephemeral_memory(self);
    name = bsal_actor_name(self);
    source = bsal_message_source(message);
    concrete_actor = (struct bsal_coverage_distribution *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_PUSH_DATA) {

        bsal_map_init(&map, 0, 0);
        bsal_map_set_memory_pool(&map, ephemeral_memory);
        bsal_map_unpack(&map, buffer);

        bsal_map_iterator_init(&iterator, &map);


        while (bsal_map_iterator_has_next(&iterator)) {

            bsal_map_iterator_next(&iterator, (void **)&coverage_from_message,
                            (void **)&count_from_message);

#ifdef BSAL_COVERAGE_DISTRIBUTION_DEBUG
            printf("DEBUG DATA %d %d\n", (int)*coverage_from_message, (int)*count_from_message);
#endif

            frequency = bsal_map_get(&concrete_actor->distribution, coverage_from_message);

            if (frequency == NULL) {

                frequency = bsal_map_add(&concrete_actor->distribution, coverage_from_message);

                (*frequency) = 0;
            }

            (*frequency) += (*count_from_message);
        }

        bsal_map_iterator_destroy(&iterator);

        bsal_actor_helper_send_reply_empty(self, BSAL_PUSH_DATA_REPLY);

        concrete_actor->actual++;

        printf("distribution/%d receives coverage data from producer/%d, %d entries / %d bytes %d/%d\n",
                        name, source, (int)bsal_map_size(&map), count,
                        concrete_actor->actual, concrete_actor->expected);

        if (concrete_actor->expected != 0 && concrete_actor->expected == concrete_actor->actual) {

            printf("received everything %d/%d\n", concrete_actor->actual, concrete_actor->expected);

            bsal_coverage_distribution_write_distribution(self);

            bsal_actor_helper_send_to_supervisor_empty(self, BSAL_SET_EXPECTED_MESSAGES_REPLY);
        }

        bsal_map_destroy(&map);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_coverage_distribution_ask_to_stop(self, message);

    } else if (tag == BSAL_SET_EXPECTED_MESSAGES) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->expected);

        printf("distribution %d expects %d messages\n",
                        bsal_actor_name(self),
                        concrete_actor->expected);
    }
}

void bsal_coverage_distribution_write_distribution(struct bsal_actor *self)
{
    struct bsal_map_iterator iterator;
    int *coverage;
    uint64_t *canonical_frequency;
    uint64_t frequency;
    struct bsal_coverage_distribution *concrete_actor;
    struct bsal_vector coverage_values;
    struct bsal_vector_iterator vector_iterator;
    FILE *descriptor;
    FILE *descriptor_canonical;
    struct bsal_string file_name;
    struct bsal_string canonical_file_name;
    int argc;
    char **argv;
    int i;
    int name;
    char default_directory[] = BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT;
    char *directory_name;

    name = bsal_actor_name(self);
    argc = bsal_actor_argc(self);
    argv = bsal_actor_argv(self);

    directory_name = default_directory;

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            directory_name = argv[i + 1];
            break;
        }
    }

    /* Create the directory if it does not exist
     */

    if (!bsal_directory_verify_existence(directory_name)) {

        bsal_directory_create(directory_name);
    }

    bsal_string_init(&file_name, "");
    bsal_string_append(&file_name, directory_name);
    bsal_string_append(&file_name, "/");
    bsal_string_append(&file_name, BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE);

    bsal_string_init(&canonical_file_name, "");
    bsal_string_append(&canonical_file_name, directory_name);
    bsal_string_append(&canonical_file_name, "/");
    bsal_string_append(&canonical_file_name, BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT_FILE_CANONICAL);

    descriptor = fopen(bsal_string_get(&file_name), "w");
    descriptor_canonical = fopen(bsal_string_get(&canonical_file_name), "w");

    concrete_actor = (struct bsal_coverage_distribution *)bsal_actor_concrete_actor(self);

    bsal_vector_init(&coverage_values, sizeof(int));
    bsal_map_iterator_init(&iterator, &concrete_actor->distribution);

#ifdef BSAL_COVERAGE_DISTRIBUTION_DEBUG
    printf("map size %d\n", (int)bsal_map_size(&concrete_actor->distribution));
#endif

    while (bsal_map_iterator_has_next(&iterator)) {
        bsal_map_iterator_next(&iterator, (void **)&coverage, (void **)&canonical_frequency);

#ifdef BSAL_COVERAGE_DISTRIBUTION_DEBUG
        printf("DEBUG COVERAGE %d FREQUENCY %" PRIu64 "\n", *coverage, *frequency);
#endif

        bsal_vector_push_back(&coverage_values, coverage);
    }

    bsal_map_iterator_destroy(&iterator);

    bsal_vector_helper_sort_int(&coverage_values);

#ifdef BSAL_COVERAGE_DISTRIBUTION_DEBUG
    printf("after sort ");
    bsal_vector_helper_print_int(&coverage_values);
    printf("\n");
#endif

    bsal_vector_iterator_init(&vector_iterator, &coverage_values);

    fprintf(descriptor, "Coverage\tFrequency\n");
#ifdef BSAL_COVERAGE_DISTRIBUTION_DEBUG
#endif

    while (bsal_vector_iterator_has_next(&vector_iterator)) {

        bsal_vector_iterator_next(&vector_iterator, (void **)&coverage);

        /*
        printf("ITERATED COVERAGE %d\n", *coverage);
        */

        canonical_frequency = (uint64_t *)bsal_map_get(&concrete_actor->distribution, coverage);

        frequency = 2 * *canonical_frequency;

        fprintf(descriptor_canonical, "%d %" PRIu64 "\n",
                        *coverage,
                        *canonical_frequency);

        fprintf(descriptor, "%d\t%" PRIu64 "\n",
                        *coverage,
                        frequency);
    }

    bsal_vector_destroy(&coverage_values);
    bsal_vector_iterator_destroy(&vector_iterator);

    printf("distribution %d wrote %s\n", name, bsal_string_get(&file_name));
    printf("distribution %d wrote %s\n", name, bsal_string_get(&canonical_file_name));

    fclose(descriptor);
    descriptor = NULL;
    fclose(descriptor_canonical);

    descriptor_canonical = NULL;

    bsal_string_destroy(&file_name);
    bsal_string_destroy(&canonical_file_name);
}

void bsal_coverage_distribution_ask_to_stop(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_map_iterator iterator;
    struct bsal_vector coverage_values;
    struct bsal_coverage_distribution *concrete_actor;

    uint64_t *frequency;
    int *coverage;

    concrete_actor = (struct bsal_coverage_distribution *)bsal_actor_concrete_actor(self);
    bsal_map_iterator_init(&iterator, &concrete_actor->distribution);

    bsal_vector_init(&coverage_values, sizeof(int));

    while (bsal_map_iterator_has_next(&iterator)) {

#if 0
        printf("DEBUG EMIT iterator\n");
#endif
        bsal_map_iterator_next(&iterator, (void **)&coverage,
                        (void **)&frequency);

#ifdef BSAL_DEBUGGER_ENABLE_ASSERT
        if (coverage == NULL) {
            printf("DEBUG map has %d buckets\n", (int)bsal_map_size(&concrete_actor->distribution));
        }
#endif
        BSAL_DEBUGGER_ASSERT(coverage != NULL);

        bsal_vector_push_back(&coverage_values, coverage);
    }

    bsal_vector_helper_sort_int(&coverage_values);

    bsal_map_iterator_destroy(&iterator);

    bsal_vector_destroy(&coverage_values);

    bsal_actor_helper_ask_to_stop(self, message);
}
