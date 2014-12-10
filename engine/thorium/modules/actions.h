
#include <engine/thorium/script.h>

/*
 ********************************************
 * In this section are listed some message tags that are available for use
 * by concrete actors.
 *
 */

/*
 * ACTOR_PING can be used by concrete actors, it is
 * not being used by biosal systems.
 */

#define PING_ACTION_BASE -3000

#define ACTION_PING (PING_ACTION_BASE + 0)
#define ACTION_PING_REPLY (PING_ACTION_BASE + 1)

/*
 * The notify messages can be used freely.
 */
#define ACTION_NOTIFY (PING_ACTION_BASE + 2)
#define ACTION_NOTIFY_REPLY (PING_ACTION_BASE + 3)

#define ACTION_RESET (PING_ACTION_BASE + 4)
#define ACTION_RESET_REPLY (PING_ACTION_BASE + 5)

#define ACTION_BEGIN (PING_ACTION_BASE + 6)
#define ACTION_BEGIN_REPLY (PING_ACTION_BASE + 7)

#define ACTION_ENABLE_MULTIPLEXER (PING_ACTION_BASE + 8)
#define ACTION_DISABLE_MULTIPLEXER (PING_ACTION_BASE + 9)

#define ACTION_ENABLE_DEFAULT_LOG_LEVEL (PING_ACTION_BASE + 10)
#define ACTION_DISABLE_DEFAULT_LOG_LEVEL (PING_ACTION_BASE + 11)

struct core_vector;
struct thorium_actor;
struct thorium_message;

void thorium_actor_add_action_with_sources(struct thorium_actor *self, int tag,
                thorium_actor_receive_fn_t handler, struct core_vector *sources);
void thorium_actor_add_action(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler);
void thorium_actor_add_action_with_source(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler,
                int source);
void thorium_actor_add_action_with_condition(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler, int *actual,
                int expected);


