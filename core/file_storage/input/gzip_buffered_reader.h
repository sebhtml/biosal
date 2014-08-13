
#ifndef BSAL_GZIP_BUFFERED_READER_H
#define BSAL_GZIP_BUFFERED_READER_H


#include "buffered_reader_interface.h"

#include <zlib.h>
#include <stdio.h>
#include <stdint.h>

struct bsal_buffered_reader;

/*
 * inflate/inflateInit is faster than gzopen/gzread/gzclose.
 */
/*
#define BSAL_GZIP_BUFFERED_READER_USE_INFLATE
*/

/*
 * A buffered reader for compressed files
 * with gzip.
 *
 * \see http://www.lemoda.net/c/zlib-open-read/
 */
struct bsal_gzip_buffered_reader {

    char *buffer;
    int buffer_capacity;
    int buffer_size;
    int position_in_buffer;

    uint64_t offset;

#ifdef BSAL_GZIP_BUFFERED_READER_USE_INFLATE
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

extern struct bsal_buffered_reader_interface bsal_gzip_buffered_reader_implementation;

void bsal_gzip_buffered_reader_init(struct bsal_buffered_reader *reader,
                const char *file, uint64_t offset);
void bsal_gzip_buffered_reader_destroy(struct bsal_buffered_reader *reader);

/*
 * \return number of bytes copied in buffer
 * This does not include the discarded \n, if any
 */
int bsal_gzip_buffered_reader_read_line(struct bsal_buffered_reader *reader,
                char *buffer, int length);

/* \return number of bytes copied in buffer
 */
int bsal_gzip_buffered_reader_pull(struct bsal_buffered_reader *reader);

void bsal_gzip_buffered_reader_open(struct bsal_buffered_reader *self,
                const char *file, uint64_t offset);

int bsal_gzip_buffered_reader_read(struct bsal_buffered_reader *self,
                char *buffer, int length);

#ifdef BSAL_GZIP_BUFFERED_READER_USE_INFLATE
int bsal_gzip_buffered_reader_pull_raw(struct bsal_buffered_reader *self);

int bsal_gzip_buffered_reader_read_with_inflate(struct bsal_buffered_reader *self,
                char *buffer, int length);
#endif

int bsal_gzip_buffered_reader_detect(struct bsal_buffered_reader *self,
                const char *file);

uint64_t bsal_gzip_buffered_reader_get_offset(struct bsal_buffered_reader *self);

int bsal_gzip_buffered_reader_read_line_private(struct bsal_buffered_reader *self,
                char *buffer, int length);
int bsal_gzip_buffered_reader_get_previous_bytes(struct bsal_buffered_reader *self,
                char *buffer, int length);

#endif
