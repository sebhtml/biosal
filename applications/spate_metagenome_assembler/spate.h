
#ifndef _SPATE_H_
#define _SPATE_H_

#include <biosal.h>

#define SPATE_SCRIPT 0xaefe198b

struct spate {
    struct bsal_vector initial_actors;
};

extern struct bsal_script spate_script;

void spate_init(struct bsal_actor *self);
void spate_destroy(struct bsal_actor *self);
void spate_receive(struct bsal_actor *self, struct bsal_message *message);

void spate_start(struct bsal_actor *self, struct bsal_message *message);

#endif
