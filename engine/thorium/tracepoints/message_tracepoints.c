
#include "message_tracepoints.h"

#include <engine/thorium/message.h>

#include <inttypes.h>
#include <stdint.h>

void thorium_tracepoint_message_actor_send(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_ACTOR_1_SEND, time);
}
