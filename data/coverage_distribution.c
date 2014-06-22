
#include "coverage_distribution.h"

#include <helpers/actor_helper.h>
#include <helpers/vector_helper.h>
#include <helpers/message_helper.h>

#include <structures/map_iterator.h>
#include <structures/vector_iterator.h>

#include <system/memory.h>

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
    int *coverage;
    uint64_t *frequency;
    int count;
    void *buffer;
    struct bsal_vector coverage_values;
    struct bsal_coverage_distribution *concrete_actor;
    int name;
    int source;

    name = bsal_actor_name(self);
    source = bsal_message_source(message);
    concrete_actor = (struct bsal_coverage_distribution *)bsal_actor_concrete_actor(self);
    tag = bsal_message_tag(message);
    count = bsal_message_count(message);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_PUSH_DATA) {

        bsal_map_unpack(&map, buffer);

        bsal_map_iterator_init(&iterator, &map);

        printf("distribution/%d receives coverage data from producer/%d, %d entries / %d bytes\n",
                        name, source, (int)bsal_map_size(&map), count);

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

        bsal_map_destroy(&map);

        bsal_actor_helper_send_reply_empty(self, BSAL_PUSH_DATA_REPLY);

        concrete_actor->actual++;

        if (concrete_actor->expected != 0 && concrete_actor->expected == concrete_actor->actual) {

            printf("received everything %d/%d\n", concrete_actor->actual, concrete_actor->expected);

            bsal_coverage_distribution_write_distribution(self);

            bsal_actor_helper_send_to_supervisor_empty(self, BSAL_SET_EXPECTED_MESSAGES_REPLY);
        }
    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        bsal_map_iterator_init(&iterator, &concrete_actor->distribution);

        bsal_vector_init(&coverage_values, sizeof(int));

        while (bsal_map_iterator_has_next(&iterator)) {

            bsal_map_iterator_next(&iterator, (void **)&coverage,
                            (void **)&count);

            bsal_vector_push_back(&coverage_values, coverage);
        }

        bsal_vector_helper_sort_int(&coverage_values);

        bsal_map_iterator_destroy(&iterator);

        bsal_vector_destroy(&coverage_values);

        bsal_actor_helper_ask_to_stop(self, message);

    } else if (tag == BSAL_SET_EXPECTED_MESSAGES) {

        bsal_message_helper_unpack_int(message, 0, &concrete_actor->expected);
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
    char *file_name;
    char *canonical_file_name;
    int argc;
    char **argv;
    int i;
    int name;
    char default_file_name[] = BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT;

    name = bsal_actor_name(self);
    argc = bsal_actor_argc(self);
    argv = bsal_actor_argv(self);

    file_name = default_file_name;

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            file_name = argv[i + 1];
            break;
        }
    }

    canonical_file_name = bsal_malloc(strlen(file_name) + 100);
    strcpy(canonical_file_name, file_name);
    strcat(canonical_file_name, "-canonical");

    descriptor = fopen(file_name, "w");
    descriptor_canonical = fopen(canonical_file_name, "w");

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

    printf("distribution actor/%d wrote %s\n", name, file_name);
    printf("distribution actor/%d wrote %s\n", name, canonical_file_name);

    fclose(descriptor);
    descriptor = NULL;
    fclose(descriptor_canonical);

    free(canonical_file_name);
    descriptor_canonical = NULL;
    canonical_file_name = NULL;
}


