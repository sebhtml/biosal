
#ifndef THORIUM_BINOMIAL_TREE_H
#define THORIUM_BINOMIAL_TREE_H

struct core_vector;
struct thorium_message;
struct thorium_actor;

/* binomial-tree */
#define ACTION_BINOMIAL_TREE_SEND 0x00005b36

void thorium_actor_receive_binomial_tree_send(struct thorium_actor *actor, struct thorium_message *message);

void thorium_actor_send_range_binomial_tree(struct thorium_actor *actor, struct core_vector *actors,
                struct thorium_message *message);

#endif
