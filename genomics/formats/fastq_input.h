
#ifndef BSAL_FASTQ_INPUT_H
#define BSAL_FASTQ_INPUT_H

#include <genomics/formats/input_format.h>
#include <genomics/formats/input_format_interface.h>

#include <core/file_storage/input/buffered_reader.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

struct bsal_fastq_input {
    struct bsal_buffered_reader reader;
    char *buffer;
};

extern struct bsal_input_format_interface bsal_fastq_input_operations;

void bsal_fastq_input_init(struct bsal_input_format *input);
void bsal_fastq_input_destroy(struct bsal_input_format *input);
uint64_t bsal_fastq_input_get_sequence(struct bsal_input_format *input,
                char *sequence);
int bsal_fastq_input_detect(struct bsal_input_format *input);

#endif
