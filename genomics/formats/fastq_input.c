
#include "fastq_input.h"

#include <core/system/memory.h>

#include <stdio.h>
#include <stdlib.h>

/*
#define BSAL_FASTQ_INPUT_DEBUG

#define BSAL_FASTQ_INPUT_DEBUG2
*/

struct bsal_input_operations bsal_fastq_input_operations = {
    .init = bsal_fastq_input_init,
    .destroy = bsal_fastq_input_destroy,
    .get_sequence = bsal_fastq_input_get_sequence,
    .detect = bsal_fastq_input_detect
};

void bsal_fastq_input_init(struct bsal_input *input)
{
    char *file;
    struct bsal_fastq_input *fastq;
    uint64_t offset;

    file = bsal_input_file(input);
    offset = bsal_input_offset(input);

#ifdef BSAL_FASTQ_INPUT_DEBUG
    printf("DEBUG bsal_fastq_input_init %s\n",
                    file);
#endif

    fastq = (struct bsal_fastq_input *)bsal_input_implementation(input);

    bsal_buffered_reader_init(&fastq->reader, file, offset);
    fastq->buffer = NULL;
}

void bsal_fastq_input_destroy(struct bsal_input *input)
{
    struct bsal_fastq_input *fastq;

    fastq = (struct bsal_fastq_input *)bsal_input_implementation(input);
    bsal_buffered_reader_destroy(&fastq->reader);

    if (fastq->buffer != NULL) {
        bsal_memory_free(fastq->buffer);
        fastq->buffer = NULL;
    }
}

uint64_t bsal_fastq_input_get_sequence(struct bsal_input *input,
                char *sequence)
{
    struct bsal_fastq_input *fastq;

    /* TODO use a dynamic buffer to accept long reads... */
    char *buffer = sequence;
    int maximum_sequence_length = BSAL_INPUT_MAXIMUM_SEQUENCE_LENGTH;
    int value;

    fastq = (struct bsal_fastq_input *)bsal_input_implementation(input);

    if (fastq->buffer == NULL) {
        fastq->buffer = (char *)bsal_memory_allocate(maximum_sequence_length + 1);
    }

    value = 0;

    value += bsal_buffered_reader_read_line(&fastq->reader, fastq->buffer,
                    maximum_sequence_length);
    value += bsal_buffered_reader_read_line(&fastq->reader, buffer,
                    maximum_sequence_length);

#ifdef BSAL_FASTQ_INPUT_DEBUG2
    printf("DEBUG bsal_fastq_input_get_sequence %s\n", buffer);
#endif

    value += bsal_buffered_reader_read_line(&fastq->reader, fastq->buffer,
                    maximum_sequence_length);
    value += bsal_buffered_reader_read_line(&fastq->reader, fastq->buffer,
                    maximum_sequence_length);

    /* add the 4 new lines if a sequence was
     * found
     */
    if (value) {
        value += 4;
    }

    return value;
}

int bsal_fastq_input_detect(struct bsal_input *input)
{
    if (bsal_input_has_suffix(input, ".fastq")) {
        return 1;
    }
    if (bsal_input_has_suffix(input, ".fq")) {
        return 1;
    }
    if (bsal_input_has_suffix(input, ".fasta-with-qualities")) {
        return 1;
    }
    if (bsal_input_has_suffix(input, ".fastq.gz")) {
        return 1;
    }
    if (bsal_input_has_suffix(input, ".fq.gz")) {
        return 1;
    }

    return 0;
}
