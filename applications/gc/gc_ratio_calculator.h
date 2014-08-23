
#ifndef _GC_RATIO_CALCULATOR_H_
#define _GC_RATIO_CALCULATOR_H_

#include <biosal.h>

#define SCRIPT_GC_RATIO_CALCULATOR 0x74c20114

#define ACTION_GC_HELLO 0x00003fa8
#define ACTION_ACTION_GC_HELLO_REPLY 0x00004790

struct gc_ratio_calculator {
    struct bsal_vector spawners;
    int completed;
};

extern struct thorium_script gc_ratio_calculator_script;

void gc_ratio_calculator_init(struct thorium_actor *actor);
void gc_ratio_calculator_destroy(struct thorium_actor *actor);
void gc_ratio_calculator_receive(struct thorium_actor *actor, struct thorium_message *message);

void gc_ratio_calculator_start(struct thorium_actor *actor, struct thorium_message *message);
void gc_ratio_calculator_hello(struct thorium_actor *actor, struct thorium_message *message);
void gc_ratio_calculator_hello_reply(struct thorium_actor *actor, struct thorium_message *message);
void gc_ratio_calculator_notify(struct thorium_actor *actor, struct thorium_message *message);
void gc_ratio_calculator_ask_to_stop(struct thorium_actor *actor, struct thorium_message *message);

#endif /* _GC_RATIO_CALCULATOR_H_ */
