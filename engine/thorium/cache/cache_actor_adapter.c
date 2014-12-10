
#include "cache_actor_adapter.h"

#include <engine/thorium/actor.h>
#include <engine/thorium/message.h>
#include <engine/thorium/worker.h>

#include <core/helpers/bitmap.h>

/*
*/
#define CONFIG_INJECT_CACHED_REPLY_MESSAGE

/*
#define CONFIG_PRINT_MESSAGE_CACHE_STATISTICS
*/

void thorium_actor_init_message_cache(struct thorium_actor *self)
{
    thorium_message_cache_init(&self->message_cache);
    thorium_message_cache_set_memory_pool(&self->message_cache,
                    &self->abstract_memory_pool);
}

void thorium_actor_destroy_message_cache(struct thorium_actor *self)
{
#ifdef CONFIG_PRINT_MESSAGE_CACHE_STATISTICS
    thorium_message_cache_print_profile(&self->message_cache);
#endif

    thorium_message_cache_destroy(&self->message_cache);
}

void thorium_actor_enable_message_cache(struct thorium_actor *self, struct thorium_message *message)
{
    int request_action;
    int reply_action;
    int count;

    CORE_BITMAP_SET_FLAG(self->flags, THORIUM_ACTOR_FLAG_ENABLE_MESSAGE_CACHE);

    count = thorium_message_count(message);

    if (count < (int)(sizeof(request_action) + sizeof(reply_action))) {
        return;
    }

    thorium_message_unpack_2_int(message, &request_action, &reply_action);

    thorium_message_cache_enable(&self->message_cache, request_action,
                    reply_action);
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
        CORE_BITMAP_CLEAR_FLAG(self->flags, THORIUM_ACTOR_FLAG_ENABLE_MESSAGE_CACHE);
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
    struct thorium_message new_message;
    int count;
    void *buffer;
    void *new_buffer;
    struct thorium_worker *worker;
    int worker_name;
    struct core_memory_pool *pool;

    /*
     * Try to get the reply message from the message cache using the request
     * message.
     */
#ifdef CONFIG_INJECT_CACHED_REPLY_MESSAGE
    reply_message = thorium_message_cache_get_reply_message(&self->message_cache, message);
#else
    reply_message = NULL;
#endif

    if (reply_message != NULL) {
        /*
         * Inject the reply message and return 1.
         */

#ifdef DISPLAY_CACHE_HIT
        printf("Found reply message in message cache !\n");
        printf("Request: ");
        thorium_message_print(message);
        printf("Reply: ");
        thorium_message_print(reply_message);
#endif

        /*
         * Make a copy of the reply_message.
         */
        core_memory_copy(&new_message, reply_message, sizeof(new_message));

        /*
         * Set the worker name for this message so that if there is
         * a buffer allocated, it can be eventually routed back here to be
         * freed.
         */
        worker = thorium_actor_worker(self);
        worker_name = thorium_worker_name(worker);
        thorium_message_set_worker(&new_message, worker_name);

        /*
         * Allocate a outbound buffer and
         * copy the data.
         */
        buffer = thorium_message_buffer(reply_message);

        if (buffer != NULL) {
            count = thorium_message_count(reply_message);

            /*
             * Use the outbound message allocator from the worker.
             */
            pool = thorium_worker_get_memory_pool(worker,
                            MEMORY_POOL_NAME_WORKER_OUTBOUND);
            new_buffer = core_memory_pool_allocate(pool, count);
            core_memory_copy(new_buffer, buffer, count);

            thorium_message_set_buffer(&new_message, new_buffer);
        }

        /*
         * And inject the message. To do so,
         * enqueue the message in the actor mailbox. If that does not work,
         * enqueue the message in the inbound queue of the worker.
         */
        if (!thorium_worker_schedule_actor(worker, self, &new_message)) {
            thorium_worker_enqueue_inbound_message_in_queue(worker, &new_message);
        }

        /*
         * Tell the calling code that a reply message was found and that it was
         * injected into the actor fabric.
         */
        return 1;
    }

    return 0;
}

void thorium_actor_save_reply_message_in_cache(struct thorium_actor *self,
                struct thorium_message *message)
{
    thorium_message_cache_save_reply_message(&self->message_cache, message);
}

void thorium_actor_print_message_cache(struct thorium_actor *self)
{
    double cache_miss_rate;
    double cache_hit_rate;
    int total;
    int profile_cache_hit_count;
    int profile_cache_miss_count;

    profile_cache_miss_count = 0;
    profile_cache_hit_count = 0;

    thorium_message_cache_get_profile(&self->message_cache, &profile_cache_miss_count,
                    &profile_cache_hit_count);

    total = 0;
    total += profile_cache_hit_count;
    total += profile_cache_miss_count;

    if (total == 0)
        return;

    cache_miss_rate = 0;
    cache_hit_rate = 0;

    if (total > 0) {
        cache_miss_rate = (0.0 + profile_cache_miss_count) / total;
        cache_hit_rate = (0.0 + profile_cache_hit_count) / total;
    }

    printf("%s/%d thorium_message_cache... cache_miss_rate %.4f cache_hit_rate %.4f total %d\n",
                    thorium_actor_script_name(self), thorium_actor_name(self),
                    cache_miss_rate, cache_hit_rate, total);
}
