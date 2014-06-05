
#ifndef _SENDER_H
#define _SENDER_H

#include <biosal.h>

#define READER_SCRIPT 0x0edc63d2

struct reader {
    struct bsal_vector spawners;
    int sequence_reader;
    int last_report;
    char *file;
    int counted;
    int pulled;
};

extern struct bsal_script reader_script;

void reader_init(struct bsal_actor *actor);
void reader_destroy(struct bsal_actor *actor);
void reader_receive(struct bsal_actor *actor, struct bsal_message *message);

#endif
