
#ifndef BSAL_ACTOR_HELPER_H
#define BSAL_ACTOR_HELPER_H

#include <engine/thorium/script.h>

#include <stdint.h>

/*
 * Actor helpers are function that work on a bsal_actor but that do not access attributes
 * with self->attribute_name
 */

struct bsal_actor;
struct bsal_vector;
struct bsal_message;

void bsal_actor_send_buffer(struct bsal_actor *actor, int destination, int tag, int count, void *buffer);
void bsal_actor_send_empty(struct bsal_actor *actor, int destination, int tag);
void bsal_actor_send_int(struct bsal_actor *actor, int destination, int tag, int value);
void bsal_actor_send_double(struct bsal_actor *actor, int destination, int tag, double value);
void bsal_actor_send_uint64_t(struct bsal_actor *actor, int destination, int tag, uint64_t value);
void bsal_actor_send_int64_t(struct bsal_actor *actor, int destination, int tag, int64_t value);
void bsal_actor_send_vector(struct bsal_actor *actor, int destination, int tag, struct bsal_vector *vector);

void bsal_actor_send_reply_empty(struct bsal_actor *actor, int tag);
void bsal_actor_send_reply_int(struct bsal_actor *actor, int tag, int value);
void bsal_actor_send_reply_int64_t(struct bsal_actor *actor, int tag, int64_t value);
void bsal_actor_send_reply_uint64_t(struct bsal_actor *actor, int tag, uint64_t value);
void bsal_actor_send_reply_vector(struct bsal_actor *actor, int tag, struct bsal_vector *vector);

void bsal_actor_send_to_self_empty(struct bsal_actor *actor, int tag);
void bsal_actor_send_to_self_int(struct bsal_actor *actor, int tag, int value);
void bsal_actor_send_to_self_buffer(struct bsal_actor *actor, int tag, int count, void *buffer);

void bsal_actor_send_to_supervisor_empty(struct bsal_actor *actor, int tag);
void bsal_actor_send_to_supervisor_int(struct bsal_actor *actor, int tag, int value);

#ifdef BSAL_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
/*
 * initialize avector and push actor names using a vector
 * of acquaintance indices
 */
void bsal_actor_get_acquaintances(struct bsal_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names);
int bsal_actor_get_acquaintance(struct bsal_actor *actor, struct bsal_vector *indices,
                int index);
int bsal_actor_get_acquaintance_index(struct bsal_actor *actor, struct bsal_vector *indices,
                int name);
void bsal_actor_add_acquaintances(struct bsal_actor *actor,
                struct bsal_vector *names, struct bsal_vector *indices);
#endif

void bsal_actor_send_range_standard(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);
/* Send a message to a range of actors.
 * The implementation uses a binomial tree.
 */
void bsal_actor_send_range(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);
void bsal_actor_send_range_int(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag, int value);
void bsal_actor_send_range_buffer(struct bsal_actor *actor, struct bsal_vector *destinations,
                int tag, int count, void *buffer);
void bsal_actor_send_range_vector(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag, struct bsal_vector *vector);
void bsal_actor_send_range_empty(struct bsal_actor *actor, struct bsal_vector *actors,
                int tag);
void bsal_actor_send_range_binomial_tree(struct bsal_actor *actor, struct bsal_vector *actors,
                struct bsal_message *message);
void bsal_actor_receive_binomial_tree_send(struct bsal_actor *actor,
                struct bsal_message *message);

void bsal_actor_ask_to_stop(struct bsal_actor *actor, struct bsal_message *message);

void bsal_actor_send_reply(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_send_to_self(struct bsal_actor *actor, struct bsal_message *message);
void bsal_actor_send_to_supervisor(struct bsal_actor *actor, struct bsal_message *message);


void bsal_actor_add_route_with_sources(struct bsal_actor *self, int tag,
                bsal_actor_receive_fn_t handler, struct bsal_vector *sources);
void bsal_actor_add_route(struct bsal_actor *self, int tag, bsal_actor_receive_fn_t handler);
void bsal_actor_add_route_with_source(struct bsal_actor *self, int tag, bsal_actor_receive_fn_t handler,
                int source);
void bsal_actor_add_route_with_condition(struct bsal_actor *self, int tag, bsal_actor_receive_fn_t handler, int *actual,
                int expected);

#endif
