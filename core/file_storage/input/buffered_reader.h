
#ifndef BSAL_BUFFERED_READER_H
#define BSAL_BUFFERED_READER_H

#include <stdio.h>
#include <stdint.h>

struct bsal_buffered_reader_interface;

/*
 * On Mira (Blue Gene/Q with GPFS file system), I/O nodes
 * lock 8-MiB blocks when reading or writing
 */
#define BSAL_BUFFERED_READER_BUFFER_SIZE 8388608

/*
 * An interface for a buffered
 * reader.
 */
struct bsal_buffered_reader {

    void *concrete_self;
    struct bsal_buffered_reader_interface *interface;
};

void bsal_buffered_reader_init(struct bsal_buffered_reader *self,
                const char *file, uint64_t offset);
void bsal_buffered_reader_destroy(struct bsal_buffered_reader *self);

/*
 * \return number of bytes copied in buffer
 * This does include the \n, if any
 */
int bsal_buffered_reader_read_line(struct bsal_buffered_reader *self,
                char *buffer, int length);

void *bsal_buffered_reader_get_concrete_self(struct bsal_buffered_reader *self);
void bsal_buffered_reader_select(struct bsal_buffered_reader *self, const char *file);
uint64_t bsal_buffered_reader_get_offset(struct bsal_buffered_reader *self);

#endif
