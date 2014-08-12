
#ifndef BSAL_FASTA_INPUT_H
#define BSAL_FASTA_INPUT_H

#include <genomics/input/input.h>
#include <genomics/input/input_operations.h>

#include <core/file_storage/input/buffered_reader.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

/*
 * A driver for fasta input.
 */
struct bsal_fasta_input {
    struct bsal_buffered_reader reader;

    char *buffer;
    char *next_header;
    int has_header;
};

extern struct bsal_input_operations bsal_fasta_input_operations;

void bsal_fasta_input_init(struct bsal_input *input);
void bsal_fasta_input_destroy(struct bsal_input *input);
uint64_t bsal_fasta_input_get_sequence(struct bsal_input *input,
                char *sequence);
int bsal_fasta_input_detect(struct bsal_input *input);

#endif
