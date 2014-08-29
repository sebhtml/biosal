
#ifndef THORIUM_ACTOR_STOP_MODULE_H
#define THORIUM_ACTOR_STOP_MODULE_H

struct thorium_actor;
struct thorium_message;
struct bsal_vector;

void thorium_actor_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message);

#ifdef THORIUM_ACTOR_EXPOSE_ACQUAINTANCE_VECTOR
/*
 * initialize avector and push actor names using a vector
 * of acquaintance indices
 */
void thorium_actor_get_acquaintances(struct thorium_actor *actor, struct bsal_vector *indices,
                struct bsal_vector *names);
int thorium_actor_get_acquaintance(struct thorium_actor *actor, struct bsal_vector *indices,
                int index);
int thorium_actor_get_acquaintance_index(struct thorium_actor *actor, struct bsal_vector *indices,
                int name);
void thorium_actor_add_acquaintances(struct thorium_actor *actor,
                struct bsal_vector *names, struct bsal_vector *indices);
#endif

#endif
