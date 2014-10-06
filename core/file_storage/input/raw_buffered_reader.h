
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

#endif
