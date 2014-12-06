
#ifndef THORIUM_CACHE_ACTOR_ADAPTER_H
#define THORIUM_CACHE_ACTOR_ADAPTER_H

/**
 * This file contains adapter functions for the message
 * cache subsystem.
 */

#define THORIUM_CACHE_BASE  -20000

#define ACTION_ENABLE_MESSAGE_CACHE (THORIUM_CACHE_BASE + 1)
#define ACTION_DISABLE_MESSAGE_CACHE (THORIUM_CACHE_BASE + 2)
#define ACTION_CLEAR_MESSAGE_CACHE (THORIUM_CACHE_BASE + 3)

struct thorium_actor;
struct thorium_message;

void thorium_actor_init_message_cache(struct thorium_actor *self);
void thorium_actor_destroy_message_cache(struct thorium_actor *self);

/**
 * Invoked by ACTION_ENABLE_MESSAGE_CACHE.
 */
void thorium_actor_enable_message_cache(struct thorium_actor *self, struct thorium_message *message);

/**
 * Invoked by ACTION_DISABLE_MESSAGE_CACHE.
 */
void thorium_actor_disable_message_cache(struct thorium_actor *self,
                struct thorium_message *message);

/**
 * Invoked by ACTION_CLEAR_MESSAGE_CACHE.
 */
void thorium_actor_clear_message_cache(struct thorium_actor *self, struct thorium_message *message);

/**
 * This hook is called in thorium_actor_receive()
 * to cache the reply message, if desired.
 */
void thorium_actor_save_reply_message_in_cache(struct  thorium_actor *self,
                struct thorium_message *message);

/**
 * This hook is called in thorium_actor_send()
 * to retrieve the reply message from the message cache.
 * If this fails, a cache tag is generated using the
 * request message and it is stored for later use.
 *
 * @return true if a reply message was successfully retrieved and
 * injected in the actor mailbox.
 */
int thorium_actor_fetch_reply_message_from_cache(struct thorium_actor *self,
                struct thorium_message *message);

#endif
