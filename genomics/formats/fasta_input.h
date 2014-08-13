
#ifndef BSAL_FASTA_INPUT_H
#define BSAL_FASTA_INPUT_H

#include <genomics/formats/input_format.h>
#include <genomics/formats/input_format_interface.h>

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

extern struct bsal_input_format_interface bsal_fasta_input_operations;

void bsal_fasta_input_init(struct bsal_input_format *self);
void bsal_fasta_input_destroy(struct bsal_input_format *self);
uint64_t bsal_fasta_input_get_sequence(struct bsal_input_format *self,
                char *sequence);
int bsal_fasta_input_detect(struct bsal_input_format *self);
uint64_t bsal_fasta_input_get_offset(struct bsal_input_format *self);

#endif
