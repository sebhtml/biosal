
#ifndef BSAL_ACTOR_HELPER_H
#define BSAL_ACTOR_HELPER_H

#include <stdint.h>

struct bsal_actor;
struct bsal_vector;
struct bsal_message;

void bsal_actor_helper_send_empty(struct bsal_actor *actor, int destination, int tag);
void bsal_actor_helper_send_int(struct bsal_actor *actor, int destination, int tag, int value);
void bsal_actor_helper_send_uint64_t(struct bsal_actor *actor, int destination, int tag, uint64_t value);
void bsal_actor_helper_send_int64_t(struct bsal_actor *actor, int destination, int tag, int64_t value);

void bsal_actor_helper_send_reply_empty(struct bsal_actor *actor, int tag);
void bsal_actor_helper_send_reply_int(struct bsal_actor *actor, int tag, int value);
void bsal_actor_helper_send_reply_int64_t(struct bsal_actor *actor, int tag, int value);
void bsal_actor_helper_send_reply_vector(struct bsal_actor *actor, int tag, struct bsal_vector *vector);

void bsal_actor_helper_send_to_self_empty(struct bsal_actor *actor, int tag);
void bsal_actor_helper_send_to_self_int(struct bsal_actor *actor, int tag, int value);

void bsal_actor_helper_send_to_supervisor_empty(struct bsal_actor *actor, int tag);
void bsal_actor_helper_send_to_supervisor_int(struct bsal_actor *actor, int tag, int value);

void bsal_actor_helper_get_acquaintances(struct bsal_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names);
void bsal_actor_helper_add_acquaintances(struct bsal_actor *actor,
                struct bsal_vector *names, struct bsal_vector *indices);

void bsal_actor_helper_send_range_standard(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);
/* Send a message to a range of actors.
 * The implementation uses a binomial tree.
 */
void bsal_actor_helper_send_range(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);
void bsal_actor_helper_send_range_int(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag, int value);
void bsal_actor_helper_send_range_empty(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag);
void bsal_actor_helper_send_range_binomial_tree(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);
void bsal_actor_helper_receive_binomial_tree_send(struct bsal_actor *actor,
                struct bsal_message *message);

#endif
