
#include "message_tracepoints.h"

#include <engine/thorium/message.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

void thorium_tracepoint_message_actor_send(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_actor_send, time);
}

void thorium_tracepoint_message_worker_send(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_worker_send, time);
}

void thorium_tracepoint_message_node_send(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_node_send, time);
}

void thorium_tracepoint_message_transport_send(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_transport_send, time);
}

void thorium_tracepoint_message_transport_receive(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_transport_receive, time);
}

void thorium_tracepoint_message_node_receive(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_node_receive, time);
}

void thorium_tracepoint_message_worker_receive(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_worker_receive, time);
}

void thorium_tracepoint_message_actor_receive(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_actor_receive, time);
}

void thorium_tracepoint_message_worker_pool_enqueue(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_worker_pool_enqueue, time);
}

void thorium_tracepoint_message_node_dispatch_message(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_node_dispatch_message, time);
}

void thorium_tracepoint_message_node_send_system(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_node_send_system, time);
}

void thorium_tracepoint_message_node_send_dispatch(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_node_send_dispatch, time);
}

void thorium_tracepoint_message_worker_enqueue_message(struct thorium_message *message, uint64_t time)
{
    /*
    printf("worker_send_enqueue %" PRIu64 "\n", time);
    */

    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_worker_enqueue_message, time);
}

void thorium_tracepoint_message_worker_send_mailbox(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_worker_send_mailbox, time);
}

void thorium_tracepoint_message_worker_send_schedule(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_worker_send_schedule, time);
}

void thorium_tracepoint_message_worker_dequeue_message(struct thorium_message *message, uint64_t time)
{
    thorium_message_set_tracepoint_time(message, THORIUM_TRACEPOINT_message_worker_dequeue_message, time);
}
