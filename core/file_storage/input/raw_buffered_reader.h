
#ifndef BSAL_RAW_BUFFERED_READER_H
#define BSAL_RAW_BUFFERED_READER_H

#include "buffered_reader_interface.h"

#include <stdio.h>
#include <stdint.h>

struct bsal_buffered_reader;

/*
 * A buffered reader for uncompressed files.
 */
struct bsal_raw_buffered_reader {

    char *buffer;
    int buffer_capacity;
    int buffer_size;
    int position_in_buffer;
    FILE *descriptor;
};

extern struct bsal_buffered_reader_interface bsal_raw_buffered_reader_implementation;

void bsal_raw_buffered_reader_init(struct bsal_buffered_reader *reader,
                const char *file, uint64_t offset);
void bsal_raw_buffered_reader_destroy(struct bsal_buffered_reader *reader);

/*
 * \return number of bytes copied in buffer
 * This does not include the discarded \n, if any
 */
int bsal_raw_buffered_reader_read_line(struct bsal_buffered_reader *reader,
                char *buffer, int length);

/* \return number of bytes copied in buffer
 */
int bsal_raw_buffered_reader_pull(struct bsal_buffered_reader *reader);

int bsal_raw_buffered_reader_detect(struct bsal_buffered_reader *self,
                const char *file);

#endif
