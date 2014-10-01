
#ifndef BIOSAL_RAW_BUFFERED_READER_H
#define BIOSAL_RAW_BUFFERED_READER_H

#include "buffered_reader_interface.h"

#include <stdio.h>
#include <stdint.h>

struct biosal_buffered_reader;

/*
 * A buffered reader for uncompressed files.
 */
struct biosal_raw_buffered_reader {

    char *buffer;
    int buffer_capacity;
    int buffer_size;
    int position_in_buffer;
    FILE *descriptor;

    uint64_t offset;
};

extern struct biosal_buffered_reader_interface biosal_raw_buffered_reader_implementation;

void biosal_raw_buffered_reader_init(struct biosal_buffered_reader *self,
                const char *file, uint64_t offset);
void biosal_raw_buffered_reader_destroy(struct biosal_buffered_reader *self);

/*
 * \return number of bytes copied in buffer
 * This does not include the discarded \n, if any
 */
int biosal_raw_buffered_reader_read_line(struct biosal_buffered_reader *self,
                char *buffer, int length);

/* \return number of bytes copied in buffer
 */
int biosal_raw_buffered_reader_pull(struct biosal_buffered_reader *self);

int biosal_raw_buffered_reader_detect(struct biosal_buffered_reader *self,
                const char *file);
uint64_t biosal_raw_buffered_reader_get_offset(struct biosal_buffered_reader *self);

int biosal_raw_buffered_reader_read_line_private(struct biosal_buffered_reader *self,
                char *buffer, int length);
int biosal_raw_buffered_reader_get_previous_bytes(struct biosal_buffered_reader *self,
                char *buffer, int length);
#endif
