
#include <engine/thorium/script.h>

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


