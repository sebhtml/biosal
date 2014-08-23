
#ifndef _SENDER_H
#define _SENDER_H

#include <biosal.h>

#define SCRIPT_READER 0x0edc63d2

struct reader {
    struct bsal_vector spawners;
    int sequence_reader;
    int last_report;
    char *file;
    int counted;
    int pulled;
};

extern struct thorium_script reader_script;

void reader_init(struct thorium_actor *self);
void reader_destroy(struct thorium_actor *self);
void reader_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
