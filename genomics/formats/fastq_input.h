
#ifndef BIOSAL_FASTQ_INPUT_H
#define BIOSAL_FASTQ_INPUT_H

#include <genomics/formats/input_format.h>
#include <genomics/formats/input_format_interface.h>

#include <core/file_storage/input/buffered_reader.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

/*
 * FastQ driver.
 */
struct core_fastq_input {
    struct core_buffered_reader reader;
    char *buffer;

    int has_first;
};

extern struct biosal_input_format_interface core_fastq_input_operations;

#endif
