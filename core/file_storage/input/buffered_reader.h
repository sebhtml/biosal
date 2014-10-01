
#ifndef CORE_BUFFERED_READER_H
#define CORE_BUFFERED_READER_H

#include <stdio.h>
#include <stdint.h>

struct core_buffered_reader_interface;

/*
 * On Mira (Blue Gene/Q with GPFS file system), I/O nodes
 * lock 8-MiB blocks when reading or writing
 */
#define CORE_BUFFERED_READER_BUFFER_SIZE 8388608

/*
 * An interface for a buffered
 * reader.
 */
struct core_buffered_reader {

    void *concrete_self;
    struct core_buffered_reader_interface *interface;
};

void core_buffered_reader_init(struct core_buffered_reader *self,
                const char *file, uint64_t offset);
void core_buffered_reader_destroy(struct core_buffered_reader *self);

/*
 * \return number of bytes copied in buffer
 * This does include the \n, if any
 */
int core_buffered_reader_read_line(struct core_buffered_reader *self,
                char *buffer, int length);

void *core_buffered_reader_get_concrete_self(struct core_buffered_reader *self);
void core_buffered_reader_select(struct core_buffered_reader *self, const char *file);
uint64_t core_buffered_reader_get_offset(struct core_buffered_reader *self);
int core_buffered_reader_get_previous_bytes(struct core_buffered_reader *self, char *buffer, int length);

#endif
