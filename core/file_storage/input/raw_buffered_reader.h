
#ifndef CORE_RAW_BUFFERED_READER_H
#define CORE_RAW_BUFFERED_READER_H

#include "buffered_reader_interface.h"

#include <stdio.h>
#include <stdint.h>

struct core_buffered_reader;

/*
 * A buffered reader for uncompressed files.
 */
struct core_raw_buffered_reader {

    char *buffer;
    int buffer_capacity;
    int buffer_size;
    int position_in_buffer;
    FILE *descriptor;

    uint64_t offset;
};

extern struct core_buffered_reader_interface core_raw_buffered_reader_implementation;

void core_raw_buffered_reader_init(struct core_buffered_reader *self,
                const char *file, uint64_t offset);
void core_raw_buffered_reader_destroy(struct core_buffered_reader *self);

/*
 * \return number of bytes copied in buffer
 * This does not include the discarded \n, if any
 */
int core_raw_buffered_reader_read_line(struct core_buffered_reader *self,
                char *buffer, int length);

/* \return number of bytes copied in buffer
 */
int core_raw_buffered_reader_pull(struct core_buffered_reader *self);

int core_raw_buffered_reader_detect(struct core_buffered_reader *self,
                const char *file);
uint64_t core_raw_buffered_reader_get_offset(struct core_buffered_reader *self);

int core_raw_buffered_reader_read_line_private(struct core_buffered_reader *self,
                char *buffer, int length);
int core_raw_buffered_reader_get_previous_bytes(struct core_buffered_reader *self,
                char *buffer, int length);
#endif
