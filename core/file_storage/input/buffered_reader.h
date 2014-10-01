
#ifndef BIOSAL_BUFFERED_READER_H
#define BIOSAL_BUFFERED_READER_H

#include <stdio.h>
#include <stdint.h>

struct biosal_buffered_reader_interface;

/*
 * On Mira (Blue Gene/Q with GPFS file system), I/O nodes
 * lock 8-MiB blocks when reading or writing
 */
#define BIOSAL_BUFFERED_READER_BUFFER_SIZE 8388608

/*
 * An interface for a buffered
 * reader.
 */
struct biosal_buffered_reader {

    void *concrete_self;
    struct biosal_buffered_reader_interface *interface;
};

void biosal_buffered_reader_init(struct biosal_buffered_reader *self,
                const char *file, uint64_t offset);
void biosal_buffered_reader_destroy(struct biosal_buffered_reader *self);

/*
 * \return number of bytes copied in buffer
 * This does include the \n, if any
 */
int biosal_buffered_reader_read_line(struct biosal_buffered_reader *self,
                char *buffer, int length);

void *biosal_buffered_reader_get_concrete_self(struct biosal_buffered_reader *self);
void biosal_buffered_reader_select(struct biosal_buffered_reader *self, const char *file);
uint64_t biosal_buffered_reader_get_offset(struct biosal_buffered_reader *self);
int biosal_buffered_reader_get_previous_bytes(struct biosal_buffered_reader *self, char *buffer, int length);

#endif
