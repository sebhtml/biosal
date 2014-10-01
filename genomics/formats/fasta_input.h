
#ifndef BIOSAL_FASTA_INPUT_H
#define BIOSAL_FASTA_INPUT_H

#include <genomics/formats/input_format.h>
#include <genomics/formats/input_format_interface.h>

#include <core/file_storage/input/buffered_reader.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

/*
 * A driver for fasta input.
 */
struct biosal_fasta_input {
    struct biosal_buffered_reader reader;

    char *buffer;
    char *next_header;
    int has_header;

    int has_first;
};

extern struct biosal_input_format_interface biosal_fasta_input_operations;

void biosal_fasta_input_init(struct biosal_input_format *self);
void biosal_fasta_input_destroy(struct biosal_input_format *self);
uint64_t biosal_fasta_input_get_sequence(struct biosal_input_format *self,
                char *sequence);
int biosal_fasta_input_detect(struct biosal_input_format *self);
uint64_t biosal_fasta_input_get_offset(struct biosal_input_format *self);

int biosal_fasta_input_check_header(struct biosal_input_format *self, const char *line);

#endif
