
#ifndef CORE_GZIP_BUFFERED_READER_H
#define CORE_GZIP_BUFFERED_READER_H


#include "buffered_reader_interface.h"

#include <zlib.h>
#include <stdio.h>
#include <stdint.h>

struct core_buffered_reader;

/*
 * inflate/inflateInit is faster than gzopen/gzread/gzclose.
 */
/*
#define CORE_GZIP_BUFFERED_READER_USE_INFLATE
*/

/*
 * A buffered reader for compressed files
 * with gzip.
 *
 * \see http://www.lemoda.net/c/zlib-open-read/
 */
struct core_gzip_buffered_reader {

    char *buffer;
    int buffer_capacity;
    int buffer_size;
    int position_in_buffer;

    uint64_t offset;

#ifdef CORE_GZIP_BUFFERED_READER_USE_INFLATE
    /*
     * \see http://www.ncbi.nlm.nih.gov/IEB/ToolBox/CPP_DOC/lxr/source/src/util/compress/zlib/example.c
     */

    FILE *raw_descriptor;

    char *input_buffer;
    int input_buffer_capacity;
    int input_buffer_size;
    int input_buffer_position;
    int got_header;
    z_stream decompression_stream;
#else

    gzFile descriptor;
#endif

};

extern struct core_buffered_reader_interface core_gzip_buffered_reader_implementation;

#endif
