
#ifndef _STREAM_H
#define _STREAM_H

#include <biosal.h>

#include <core/structures/vector.h>

#define SCRIPT_STREAM 0xb9b19139

struct stream {
    struct core_vector spawners;
    int initial_synchronization;
    int ready;
    struct core_vector children;
    int is_king;
};

#define ACTION_STREAM_DIE 0x00005988
#define ACTION_STREAM_READY 0x000077cc
#define ACTION_STREAM_SYNC 0x00006fed

extern struct thorium_script stream_script;

#endif
