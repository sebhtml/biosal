
#ifndef BSAL_MESSAGE_HELPER_H
#define BSAL_MESSAGE_HELPER_H

struct bsal_message;

int bsal_message_helper_unpack_int(struct bsal_message *message, int offset, int *value);
void bsal_message_helper_get_all(struct bsal_message *message, int *tag, int *count, void **buffer, int *source);

#endif
