
#ifndef FUTURE_123
#define FUTURE_123

#include <engine/thorium/actor.h>

struct thorium_actor;
struct thorium_message;

void thorium_actor_send_reply_then(struct thorium_actor *self,
                struct thorium_message *message, thorium_actor_receive_fn_t handler);
void thorium_actor_send_then(struct thorium_actor *self, int destination,
                struct thorium_message *message, thorium_actor_receive_fn_t handler);

#endif
