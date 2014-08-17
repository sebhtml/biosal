
#ifndef BSAL_MESSAGE_HELPER_H
#define BSAL_MESSAGE_HELPER_H

#include <stdint.h>

struct thorium_message;

int thorium_message_unpack_int(struct thorium_message *message, int offset, int *value);
int thorium_message_unpack_double(struct thorium_message *message, int offset, double *value);
int thorium_message_unpack_uint64_t(struct thorium_message *message, int offset, uint64_t *value);
int thorium_message_unpack_int64_t(struct thorium_message *message, int offset, int64_t *value);
void thorium_message_get_all(struct thorium_message *message, int *tag, int *count, void **buffer, int *source);

#endif
