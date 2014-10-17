
#ifndef THORIUM_MESSAGE_TRACEPOINTS_H
#define THORIUM_MESSAGE_TRACEPOINTS_H

/*
 * Thorium tracepoints are used to gather information about events.
 * Tracepoint information can be embedded in actor messages.
 *
 * \see https://github.com/giraldeau/lttng-ust/blob/master/include/lttng/tracepoint.h
 */
#include "tracepoints.h"

#include <stdint.h>

struct thorium_message;

void thorium_tracepoint_message_actor_send(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_worker_send(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_node_send(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_transport_send(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_transport_receive(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_node_receive(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_worker_receive(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_actor_receive(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_worker_pool_enqueue(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_node_dispatch_message(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_node_send_system(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_node_send_dispatch(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_worker_enqueue_message(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_worker_send_mailbox(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_worker_send_schedule(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_worker_dequeue_message(uint64_t time, struct thorium_message *message);
void thorium_tracepoint_message_worker_pool_dequeue(uint64_t time, struct thorium_message *message);

#endif
