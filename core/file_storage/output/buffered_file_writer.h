#ifndef BSAL_BUFFERED_FILE_WRITER_H
#define BSAL_BUFFERED_FILE_WRITER_H

#include <stdio.h>
#include <stdarg.h>

/*
 * A buffered file writer.
 */
struct bsal_buffered_file_writer {
    FILE *descriptor;
    char *buffer;
    char *format_buffer;
    int format_buffer_capacity;
    int buffer_capacity;
    int buffer_length;

    FILE *null_file;
};

void bsal_buffered_file_writer_init(struct bsal_buffered_file_writer *self, const char *file);
void bsal_buffered_file_writer_destroy(struct bsal_buffered_file_writer *self);

int bsal_buffered_file_writer_write(struct bsal_buffered_file_writer *self,
                const char *data, int length);
int bsal_buffered_file_writer_printf(struct bsal_buffered_file_writer *self, const char *format,
                ...);

int bsal_buffered_file_writer_flush(struct bsal_buffered_file_writer *self);

#endif
