
#include "actor_tracepoints.h"

#include <engine/thorium/actor_profiler.h>

void thorium_tracepoint_actor_receive_enter(struct thorium_actor_profiler *profiler,
                struct thorium_message *message)
{
    thorium_actor_profiler_profile(profiler, THORIUM_TRACEPOINT_actor_receive_enter,
                        message);
}

void thorium_tracepoint_actor_receive_exit(struct thorium_actor_profiler *profiler,
                struct thorium_message *message)
{
    thorium_actor_profiler_profile(profiler, THORIUM_TRACEPOINT_actor_receive_exit,
                        message);
}



