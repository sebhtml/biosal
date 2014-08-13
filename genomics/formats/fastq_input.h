
#ifndef BSAL_FASTQ_INPUT_H
#define BSAL_FASTQ_INPUT_H

#include <genomics/formats/input_format.h>
#include <genomics/formats/input_format_interface.h>

#include <core/file_storage/input/buffered_reader.h>

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

/*
 * FastQ driver.
 */
struct bsal_fastq_input {
    struct bsal_buffered_reader reader;
    char *buffer;

    int has_first;
};

extern struct bsal_input_format_interface bsal_fastq_input_operations;

void bsal_fastq_input_init(struct bsal_input_format *self);
void bsal_fastq_input_destroy(struct bsal_input_format *self);
uint64_t bsal_fastq_input_get_sequence(struct bsal_input_format *self,
                char *sequence);
int bsal_fastq_input_detect(struct bsal_input_format *self);
uint64_t bsal_fastq_input_get_offset(struct bsal_input_format *self);

int bsal_fastq_input_is_identifier(struct bsal_input_format *self, const char *line);
int bsal_fastq_input_is_identifier_mock(struct bsal_input_format *self, const char *line);

#endif
