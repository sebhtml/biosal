
#ifndef _GC_RATIO_CALCULATOR_H_
#define _GC_RATIO_CALCULATOR_H_

#include <biosal.h>

#define GC_RATIO_CALCULATOR_SCRIPT 0x74c20114

#define GC_HELLO 0x00003fa8
#define GC_HELLO_REPLY 0x00004790

struct gc_ratio_calculator {
    struct bsal_vector spawners;
    int completed;
};

extern struct bsal_script gc_ratio_calculator_script;

void gc_ratio_calculator_init(struct bsal_actor *actor);
void gc_ratio_calculator_destroy(struct bsal_actor *actor);
void gc_ratio_calculator_receive(struct bsal_actor *actor, struct bsal_message *message);

void gc_ratio_calculator_start(struct bsal_actor *actor, struct bsal_message *message);
void gc_ratio_calculator_hello(struct bsal_actor *actor, struct bsal_message *message);
void gc_ratio_calculator_hello_reply(struct bsal_actor *actor, struct bsal_message *message);
void gc_ratio_calculator_notify(struct bsal_actor *actor, struct bsal_message *message);
void gc_ratio_calculator_ask_to_stop(struct bsal_actor *actor, struct bsal_message *message);

#endif /* _GC_RATIO_CALCULATOR_H_ */
