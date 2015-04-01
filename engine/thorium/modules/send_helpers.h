
#ifndef THORIUM_ACTOR_HELPER_H
#define THORIUM_ACTOR_HELPER_H

#include <engine/thorium/script.h>

#include <stdint.h>

#define SEND_HELPERS_ACTION_BASE -5000
#define ACTION_SET_PRODUCERS_FOR_WORK_STEALING (SEND_HELPERS_ACTION_BASE + 0)
#define ACTION_SET_PRODUCERS_FOR_WORK_STEALING_REPLY (SEND_HELPERS_ACTION_BASE + 1)

/*
 * Actor helpers are function that work on a thorium_actor but that do not access attributes
 * with self->attribute_name
 */

struct thorium_actor;
struct core_vector;
struct thorium_message;

void thorium_actor_send_buffer(struct thorium_actor *actor, int destination, int action, int count, void *buffer);
void thorium_actor_send_empty(struct thorium_actor *actor, int destination, int action);
void thorium_actor_send_int(struct thorium_actor *actor, int destination, int action, int value);
void thorium_actor_send_2_int(struct thorium_actor *actor, int destination, int action, int value1, int value2);
void thorium_actor_send_double(struct thorium_actor *actor, int destination, int action, double value);
void thorium_actor_send_uint64_t(struct thorium_actor *actor, int destination, int action, uint64_t value);
void thorium_actor_send_int64_t(struct thorium_actor *actor, int destination, int action, int64_t value);
void thorium_actor_send_vector(struct thorium_actor *actor, int destination, int action, struct core_vector *vector);

void thorium_actor_send_reply_empty(struct thorium_actor *actor, int action);
void thorium_actor_send_reply_int(struct thorium_actor *actor, int action, int value);
void thorium_actor_send_reply_int64_t(struct thorium_actor *actor, int action, int64_t value);
void thorium_actor_send_reply_uint64_t(struct thorium_actor *actor, int action, uint64_t value);
void thorium_actor_send_reply_vector(struct thorium_actor *actor, int action, struct core_vector *vector);
void thorium_actor_send_reply_buffer(struct thorium_actor *actor, int action, int count, void *buffer);

void thorium_actor_send_to_self_empty(struct thorium_actor *actor, int action);
void thorium_actor_send_to_self_int(struct thorium_actor *actor, int action, int value);
void thorium_actor_send_to_self_2_int(struct thorium_actor *actor, int action, int value1, int value2);
void thorium_actor_send_to_self_buffer(struct thorium_actor *actor, int action, int count, void *buffer);

void thorium_actor_send_to_supervisor_empty(struct thorium_actor *actor, int action);
void thorium_actor_send_to_supervisor_int(struct thorium_actor *actor, int action, int value);

void thorium_actor_send_range_default(struct thorium_actor *actor, struct core_vector *actors,
                int first, int last,
                struct thorium_message *message);

void thorium_actor_send_range(struct thorium_actor *actor, struct core_vector *actors,
                struct thorium_message *message);
void thorium_actor_send_range_int(struct thorium_actor *actor, struct core_vector *actors,
                int action, int value);
void thorium_actor_send_range_buffer(struct thorium_actor *actor, struct core_vector *destinations,
                int action, int count, void *buffer);
void thorium_actor_send_range_vector(struct thorium_actor *actor, struct core_vector *actors,
                int action, struct core_vector *vector);
void thorium_actor_send_range_empty(struct thorium_actor *actor, struct core_vector *actors,
                int action);
void thorium_actor_send_range_loop(struct thorium_actor *actor, struct core_vector *actors,
                int first, int last,
                struct thorium_message *message);

void thorium_actor_send_range_positions_vector(struct thorium_actor *actor, struct core_vector *actors,
                int first, int last,
                int action, struct core_vector *vector);

void thorium_actor_send_reply(struct thorium_actor *actor, struct thorium_message *message);
void thorium_actor_send_to_self(struct thorium_actor *actor, struct thorium_message *message);
void thorium_actor_send_to_supervisor(struct thorium_actor *actor, struct thorium_message *message);

void thorium_actor_send_int_vector(struct thorium_actor *self, int dst, int action, int value1, struct core_vector *value2);

#endif
