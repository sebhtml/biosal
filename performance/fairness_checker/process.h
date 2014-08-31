
#ifndef PROCESS_H
#define PROCESS_H

#include <biosal.h>

#define SCRIPT_FAIRNESS_PROCESS 0x01412af4

struct process {
    struct bsal_vector times;
    struct bsal_vector actors;
    struct bsal_timer timer;
    int ready;
};

extern struct thorium_script process_script;

void process_init(struct thorium_actor *self);
void process_destroy(struct thorium_actor *self);
void process_receive(struct thorium_actor *self, struct thorium_message *message);

void process_start(struct thorium_actor *self, struct thorium_message *message);
void process_stop(struct thorium_actor *self, struct thorium_message *message);
void process_ping_reply(struct thorium_actor *self, struct thorium_message *message);
void process_send_ping(struct thorium_actor *self);
void process_notify(struct thorium_actor *self, struct thorium_message *message);
void process_ping(struct thorium_actor *self, struct thorium_message *message);

#endif
