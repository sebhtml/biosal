
#ifndef BSAL_GZIP_BUFFERED_READER_H
#define BSAL_GZIP_BUFFERED_READER_H

struct bsal_buffered_reader;

#include <zlib.h>
#include <stdio.h>
#include <stdint.h>

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
    gzFile descriptor;
};

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

#endif
