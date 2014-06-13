
#include "message_helper.h"

#include <engine/message.h>

int bsal_message_helper_unpack_int(struct bsal_message *message, int offset, int *value)
{
    int bytes;
    void *buffer;
    int *pointer;

    if (offset >= bsal_message_count(message)) {
        return -1;
    }

    bytes = sizeof(value);
    buffer = bsal_message_buffer(message);

    pointer = (int *)((char *)buffer + offset);

    *value = *pointer;

    offset += bytes;

    return offset;
}

void bsal_message_helper_get_all(struct bsal_message *message, int *tag, int *count, void **buffer, int *source)
{
    *tag = bsal_message_tag(message);
    *count = bsal_message_count(message);
    *buffer = bsal_message_buffer(message);
    *source = bsal_message_source(message);
}


