
#ifndef BSAL_ACTOR_HELPER_H
#define BSAL_ACTOR_HELPER_H

struct bsal_actor;
struct bsal_vector;

void bsal_actor_helper_send_empty(struct bsal_actor *actor, int destination, int tag);
void bsal_actor_helper_send_int(struct bsal_actor *actor, int destination, int tag, int value);

void bsal_actor_helper_send_reply_empty(struct bsal_actor *actor, int tag);
void bsal_actor_helper_send_reply_int(struct bsal_actor *actor, int tag, int error);
void bsal_actor_helper_send_reply_vector(struct bsal_actor *actor, int tag, struct bsal_vector *vector);

void bsal_actor_helper_send_to_self_empty(struct bsal_actor *actor, int tag);
void bsal_actor_helper_send_to_self_int(struct bsal_actor *actor, int tag, int value);

void bsal_actor_helper_send_to_supervisor_empty(struct bsal_actor *actor, int tag);
void bsal_actor_helper_send_to_supervisor_int(struct bsal_actor *actor, int tag, int value);

void bsal_actor_helper_get_acquaintances(struct bsal_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names);
void bsal_actor_helper_add_acquaintances(struct bsal_actor *actor,
                struct bsal_vector *names, struct bsal_vector *indices);
#endif
