
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
struct core_fasta_input {
    struct core_buffered_reader reader;

    char *buffer;
    char *next_header;
    int has_header;

    int has_first;
};

extern struct biosal_input_format_interface core_fasta_input_operations;

#endif
