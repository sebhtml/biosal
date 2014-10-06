
#ifndef _SENDER_H
#define _SENDER_H

#include <biosal.h>

#define SCRIPT_READER 0x0edc63d2

struct reader {
    struct core_vector spawners;
    int sequence_reader;
    int last_report;
    char *file;
    int counted;
    int pulled;
};

extern struct thorium_script reader_script;

#endif
