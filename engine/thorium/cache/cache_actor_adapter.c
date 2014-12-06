
#include "cache_actor_adapter.h"

#include <engine/thorium/actor.h>
#include <engine/thorium/message.h>

#include <core/helpers/bitmap.h>

void thorium_actor_enable_message_cache(struct thorium_actor *self, struct thorium_message *message)
{
    int cache_action;
    char *buffer;

    CORE_BITMAP_SET_BIT(self->flags, THORIUM_ACTOR_FLAG_ENABLE_MESSAGE_CACHE);

    buffer = thorium_message_buffer(message);
    cache_action = *(int *)buffer;
    thorium_message_cache_enable(&self->message_cache, cache_action);
}

void thorium_actor_disable_message_cache(struct thorium_actor *self,
                struct thorium_message *message)
{
    int cache_action;
    char *buffer;

    buffer = thorium_message_buffer(message);
    cache_action = *(int *)buffer;

    thorium_message_cache_disable(&self->message_cache, cache_action);

    /*
     * Set the flag THORIUM_ACTOR_FLAG_ENABLE_MESSAGE_CACHE to 0 when no actions are
     * configured to use the message cache subsystem.
     */
    if (thorium_message_cache_action_count(&self->message_cache) == 0) {
        CORE_BITMAP_CLEAR_BIT(self->flags, THORIUM_ACTOR_FLAG_ENABLE_MESSAGE_CACHE);
    }
}

void thorium_actor_clear_message_cache(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_message_cache_clear(&self->message_cache);
}

int thorium_actor_fetch_reply_message_from_cache(struct thorium_actor *self,
                struct thorium_message *message)
{
    struct thorium_message *reply_message;

    /*
     * Try to get the reply message from the message cache using the request
     * message.
     */
    reply_message = thorium_message_cache_get_reply_message(&self->message_cache, message);

    if (reply_message != NULL) {
        /*
         * Inject the reply message and return 1.
         */
        /* TODO */

#ifdef DISPLAY_CACHE_HIT
        printf("Found reply message in message cache !\n");
        printf("Request: ");
        thorium_message_print(message);
        printf("Reply: ");
        thorium_message_print(reply_message);
#endif
    }

    return 0;
}
