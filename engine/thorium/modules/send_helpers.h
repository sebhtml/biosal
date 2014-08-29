
#ifndef THORIUM_ACTOR_HELPER_H
#define THORIUM_ACTOR_HELPER_H

#include <engine/thorium/script.h>

#include <stdint.h>

#define ACTION_SET_PRODUCERS_FOR_WORK_STEALING 0x002f77ab
#define ACTION_SET_PRODUCERS_FOR_WORK_STEALING_REPLY 0x0038c869

/*
 * Actor helpers are function that work on a thorium_actor but that do not access attributes
 * with self->attribute_name
 */

struct thorium_actor;
struct bsal_vector;
struct thorium_message;

void thorium_actor_send_buffer(struct thorium_actor *actor, int destination, int tag, int count, void *buffer);
void thorium_actor_send_empty(struct thorium_actor *actor, int destination, int tag);
void thorium_actor_send_int(struct thorium_actor *actor, int destination, int tag, int value);
void thorium_actor_send_double(struct thorium_actor *actor, int destination, int tag, double value);
void thorium_actor_send_uint64_t(struct thorium_actor *actor, int destination, int tag, uint64_t value);
void thorium_actor_send_int64_t(struct thorium_actor *actor, int destination, int tag, int64_t value);
void thorium_actor_send_vector(struct thorium_actor *actor, int destination, int tag, struct bsal_vector *vector);

void thorium_actor_send_reply_empty(struct thorium_actor *actor, int tag);
void thorium_actor_send_reply_int(struct thorium_actor *actor, int tag, int value);
void thorium_actor_send_reply_int64_t(struct thorium_actor *actor, int tag, int64_t value);
void thorium_actor_send_reply_uint64_t(struct thorium_actor *actor, int tag, uint64_t value);
void thorium_actor_send_reply_vector(struct thorium_actor *actor, int tag, struct bsal_vector *vector);

void thorium_actor_send_to_self_empty(struct thorium_actor *actor, int tag);
void thorium_actor_send_to_self_int(struct thorium_actor *actor, int tag, int value);
void thorium_actor_send_to_self_buffer(struct thorium_actor *actor, int tag, int count, void *buffer);

void thorium_actor_send_to_supervisor_empty(struct thorium_actor *actor, int tag);
void thorium_actor_send_to_supervisor_int(struct thorium_actor *actor, int tag, int value);

void thorium_actor_send_range_default(struct thorium_actor *actor, struct bsal_vector *actors,
                int first, int last,
                struct thorium_message *message);

void thorium_actor_send_range(struct thorium_actor *actor, struct bsal_vector *actors,
                struct thorium_message *message);
void thorium_actor_send_range_int(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag, int value);
void thorium_actor_send_range_buffer(struct thorium_actor *actor, struct bsal_vector *destinations,
                int tag, int count, void *buffer);
void thorium_actor_send_range_vector(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag, struct bsal_vector *vector);
void thorium_actor_send_range_empty(struct thorium_actor *actor, struct bsal_vector *actors,
                int tag);
void thorium_actor_send_range_loop(struct thorium_actor *actor, struct bsal_vector *actors,
                int first, int last,
                struct thorium_message *message);

void thorium_actor_send_range_positions_vector(struct thorium_actor *actor, struct bsal_vector *actors,
                int first, int last,
                int tag, struct bsal_vector *vector);

void thorium_actor_send_reply(struct thorium_actor *actor, struct thorium_message *message);
void thorium_actor_send_to_self(struct thorium_actor *actor, struct thorium_message *message);
void thorium_actor_send_to_supervisor(struct thorium_actor *actor, struct thorium_message *message);

#endif
