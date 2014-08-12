
#ifndef BSAL_BUFFERED_READER_H
#define BSAL_BUFFERED_READER_H

#include <stdio.h>
#include <stdint.h>

struct bsal_buffered_reader_interface;

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
 * This does not include the discarded \n, if any
 */
int bsal_buffered_reader_read_line(struct bsal_buffered_reader *self,
                char *buffer, int length);

void *bsal_buffered_reader_get_concrete_self(struct bsal_buffered_reader *self);
void bsal_buffered_reader_select(struct bsal_buffered_reader *self, const char *file);

#endif
