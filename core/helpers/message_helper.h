
#ifndef BSAL_MESSAGE_HELPER_H
#define BSAL_MESSAGE_HELPER_H

#include <stdint.h>

struct bsal_message;

int bsal_message_unpack_int(struct bsal_message *message, int offset, int *value);
int bsal_message_unpack_double(struct bsal_message *message, int offset, double *value);
int bsal_message_unpack_uint64_t(struct bsal_message *message, int offset, uint64_t *value);
int bsal_message_unpack_int64_t(struct bsal_message *message, int offset, int64_t *value);
void bsal_message_get_all(struct bsal_message *message, int *tag, int *count, void **buffer, int *source);

#endif
