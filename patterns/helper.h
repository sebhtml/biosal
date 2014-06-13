
#ifndef BSAL_HELPER_H
#define BSAL_HELPER_H

#include <engine/actor.h>
#include <structures/vector.h>

struct bsal_helper {
    int mock;
};

void bsal_helper_init(struct bsal_helper *self);
void bsal_helper_destroy(struct bsal_helper *self);

void bsal_helper_send_empty(struct bsal_actor *actor, int destination, int tag);
void bsal_helper_send_int(struct bsal_actor *actor, int destination, int tag, int value);

void bsal_helper_send_reply_empty(struct bsal_actor *actor, int tag);
void bsal_helper_send_reply_int(struct bsal_actor *actor, int tag, int error);
void bsal_helper_send_reply_vector(struct bsal_actor *actor, int tag, struct bsal_vector *vector);

void bsal_helper_send_to_self_empty(struct bsal_actor *actor, int tag);
void bsal_helper_send_to_self_int(struct bsal_actor *actor, int tag, int value);

void bsal_helper_send_to_supervisor_empty(struct bsal_actor *actor, int tag);
void bsal_helper_send_to_supervisor_int(struct bsal_actor *actor, int tag, int value);

int bsal_helper_message_unpack_int(struct bsal_message *message, int offset, int *value);
void bsal_helper_message_get_all(struct bsal_message *message, int *tag, int *count, void **buffer, int *source);

void bsal_helper_vector_print_int(struct bsal_vector *self);
void bsal_helper_vector_set_int(struct bsal_vector *self, int64_t index, int value);
void bsal_helper_vector_push_back_int(struct bsal_vector *self, int value);
int bsal_helper_vector_at_as_int(struct bsal_vector *self, int64_t index);
char *bsal_helper_vector_at_as_char_pointer(struct bsal_vector *self, int64_t index);
void *bsal_helper_vector_at_as_void_pointer(struct bsal_vector *self, int64_t index);

#endif
