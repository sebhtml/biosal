
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
#define ACTION_PING 0x000040b3
#define ACTION_PING_REPLY 0x00006eda

/*
 * The notify messages can be used freely.
 */
#define ACTION_NOTIFY 0x0000710b
#define ACTION_NOTIFY_REPLY 0x00005f82

#define ACTION_RESET 0x00005045
#define ACTION_RESET_REPLY 0x0000056d

#define ACTION_BEGIN 0x0000125f
#define ACTION_BEGIN_REPLY 0x0000214a

struct bsal_vector;
struct thorium_actor;
struct thorium_message;

void thorium_actor_add_action_with_sources(struct thorium_actor *self, int tag,
                thorium_actor_receive_fn_t handler, struct bsal_vector *sources);
void thorium_actor_add_action(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler);
void thorium_actor_add_action_with_source(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler,
                int source);
void thorium_actor_add_action_with_condition(struct thorium_actor *self, int tag, thorium_actor_receive_fn_t handler, int *actual,
                int expected);


