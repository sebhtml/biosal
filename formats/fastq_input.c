
#include "fastq_input.h"

#include <stdio.h>

struct bsal_input_vtable bsal_fastq_input_vtable = {
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

    printf("DEBUG bsal_fastq_input_init %s\n",
                    file);

    fastq = (struct bsal_fastq_input *)bsal_input_pointer(input);
    fastq->descriptor = fopen(file, "r");
    fastq->dummy = 0;
}

void bsal_fastq_input_destroy(struct bsal_input *input)
{
    struct bsal_fastq_input *fastq;

    fastq = (struct bsal_fastq_input *)bsal_input_pointer(input);
    fclose(fastq->descriptor);
    fastq->dummy = -1;
    fastq->descriptor = NULL;
}

int bsal_fastq_input_get_sequence(struct bsal_input *input,
                struct bsal_dna_sequence *sequence)
{
    struct bsal_fastq_input *fastq;

    fastq = (struct bsal_fastq_input *)bsal_input_pointer(input);
    fastq->dummy++;

    if (fastq->dummy < 1555999) {
        return 1;
    }

    return 0;
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
