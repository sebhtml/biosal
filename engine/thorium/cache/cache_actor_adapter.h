
#ifndef THORIUM_CACHE_ACTOR_ADAPTER_H
#define THORIUM_CACHE_ACTOR_ADAPTER_H

struct thorium_actor;
struct thorium_message;

void thorium_actor_enable_message_cache(struct thorium_actor *self, struct thorium_message *message);
void thorium_actor_disable_message_cache(struct thorium_actor *self,
                struct thorium_message *message);
void thorium_actor_clear_message_cache(struct thorium_actor *self, struct thorium_message *message);

int thorium_actor_fetch_reply_message_from_cache(struct thorium_actor *self,
                struct thorium_message *message);

#endif
