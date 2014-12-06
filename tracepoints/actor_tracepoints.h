
#ifndef THORIUM_ACTOR_TRACEPOINT_H
#define THORIUM_ACTOR_TRACEPOINT_H

struct thorium_actor_profiler;
struct thorium_message;

#include <stdint.h>

/*
 * tracepoint event "actor:receive_enter"
 */
void thorium_tracepoint_actor_receive_enter(uint64_t time,
                struct thorium_actor_profiler *profiler,
                struct thorium_message *message);

/*
 * tracepoint event "actor:receive_exit"
 */
void thorium_tracepoint_actor_receive_exit(uint64_t time,
                struct thorium_actor_profiler *profiler,
                struct thorium_message *message);

#endif
