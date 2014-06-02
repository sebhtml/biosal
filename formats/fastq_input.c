
#include "fastq_input.h"

#include <data/dna_sequence.h>

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

    file = bsal_input_file(input);

#ifdef BSAL_FASTQ_INPUT_DEBUG
    printf("DEBUG bsal_fastq_input_init %s\n",
                    file);
#endif

    fastq = (struct bsal_fastq_input *)bsal_input_implementation(input);

    bsal_buffered_reader_init(&fastq->reader, file);
}

void bsal_fastq_input_destroy(struct bsal_input *input)
{
    struct bsal_fastq_input *fastq;

    fastq = (struct bsal_fastq_input *)bsal_input_implementation(input);
    bsal_buffered_reader_destroy(&fastq->reader);
}

int bsal_fastq_input_get_sequence(struct bsal_input *input,
                struct bsal_dna_sequence *sequence)
{
    struct bsal_fastq_input *fastq;

    /* TODO use a dynamic buffer to accept long reads... */
    char buffer[2048];
    int value;

    fastq = (struct bsal_fastq_input *)bsal_input_implementation(input);

    value = bsal_buffered_reader_read_line(&fastq->reader, buffer, 2048);
    value = bsal_buffered_reader_read_line(&fastq->reader, buffer, 2048);

#ifdef BSAL_FASTQ_INPUT_DEBUG2
    printf("DEBUG bsal_fastq_input_get_sequence %s\n", buffer);
#endif

    bsal_dna_sequence_init(sequence, buffer);

    value = bsal_buffered_reader_read_line(&fastq->reader, buffer, 2048);
    value = bsal_buffered_reader_read_line(&fastq->reader, buffer, 2048);

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

    return 0;
}
