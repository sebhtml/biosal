
#include "transport_profiler.h"

#include <engine/thorium/message.h>

#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/map_iterator.h>

#include <stdio.h>

void thorium_transport_profiler_init(struct thorium_transport_profiler *self)
{
    self->rank = -1;
    biosal_map_init(&self->buffer_sizes, sizeof(int), sizeof(int));
}

void thorium_transport_profiler_destroy(struct thorium_transport_profiler *self)
{
    self->rank = -1;
    biosal_map_destroy(&self->buffer_sizes);
}

void thorium_transport_profiler_print_report(struct thorium_transport_profiler *self)
{
    struct biosal_map_iterator iterator;
    struct biosal_vector_iterator vector_iterator;
    int buffer_size;
    int frequency;
    struct biosal_vector sizes;

    printf("Thorium Transport Profiler Report, node/%d\n",
                    self->rank);

    biosal_vector_init(&sizes, sizeof(int));
    biosal_map_iterator_init(&iterator, &self->buffer_sizes);

    while (biosal_map_iterator_get_next_key_and_value(&iterator,
                            &buffer_size, NULL)) {

        biosal_vector_push_back(&sizes, &buffer_size);
    }

    biosal_vector_sort_int(&sizes);
    biosal_map_iterator_destroy(&iterator);

    biosal_vector_iterator_init(&vector_iterator, &sizes);

    printf("Buffer sizes:\n");
    printf("ByteCount\tFrequency\n");

    while (biosal_vector_iterator_get_next_value(&vector_iterator,
                            &buffer_size)) {

        biosal_map_get_value(&self->buffer_sizes, &buffer_size, &frequency);

        printf("%d\t%d\n", buffer_size, frequency);
    }

    biosal_vector_iterator_destroy(&vector_iterator);
    biosal_vector_destroy(&sizes);
}

void thorium_transport_profiler_send_mock(struct thorium_transport_profiler *self,
                struct thorium_message *message)
{
    int rank;
    int size;
    int *bucket;

    size = thorium_message_count(message);
    rank = thorium_message_source_node(message);

    if (self->rank < 0) {
        self->rank = rank;
    }

    bucket = biosal_map_get(&self->buffer_sizes, &size);

    if (bucket == NULL) {
        bucket = biosal_map_add(&self->buffer_sizes, &size);
        *bucket = 0;
    }

    ++(*bucket);
}
