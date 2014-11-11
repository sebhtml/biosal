
#include "multiplexed_buffer.h"

#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>

void thorium_multiplexed_buffer_print(struct thorium_multiplexed_buffer *self)
{
    printf("multiplexed_buffer_print buffer %p timestamp %" PRIu64 " current_size %d"
                    " maximum_size %d message_count %d"
                    "\n", self->buffer, self->timestamp,
                    self->current_size, self->maximum_size,
                    self->message_count);
}
