
#include "buffered_reader.h"

#include <stdio.h>
#include <stdlib.h>

void bsal_buffered_reader_init(struct bsal_buffered_reader *reader,
                const char *file)
{
    reader->descriptor = fopen(file, "r");

    reader->buffer = (char *)malloc(BSAL_INPUT_BUFFER_SIZE * sizeof(char));
    reader->buffer_capacity = BSAL_INPUT_BUFFER_SIZE;
    reader->position_in_buffer = 0;
    reader->buffer_size = 0;

    reader->dummy = 0;
}

void bsal_buffered_reader_destroy(struct bsal_buffered_reader *reader)
{
    reader->buffer = NULL;
    reader->buffer_capacity = 0;
    reader->buffer_size = 0;
    reader->position_in_buffer = 0;

    fclose(reader->descriptor);
    reader->descriptor = NULL;
}

int bsal_buffered_reader_read_line(struct bsal_buffered_reader *reader,
                char *buffer, int length)
{
    if (reader->dummy == 1222888) {
        return 0;
    }

    reader->dummy++;

    return 1;
}
