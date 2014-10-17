
#ifndef THORIUM_NODE_TRACEPOINTS_H
#define THORIUM_NODE_TRACEPOINTS_H

#include "tracepoints.h"

#include <stdint.h>

struct thorium_tracepoint_session;

void thorium_tracepoint_node_run_loop_print(uint64_t time, struct thorium_tracepoint_session *tracepoint_session);
void thorium_tracepoint_node_run_loop_receive(uint64_t time, struct thorium_tracepoint_session *tracepoint_session);
void thorium_tracepoint_node_run_loop_run(uint64_t time, struct thorium_tracepoint_session *tracepoint_session);
void thorium_tracepoint_node_run_loop_send(uint64_t time, struct thorium_tracepoint_session *tracepoint_session);
void thorium_tracepoint_node_run_loop_pool_work(uint64_t time, struct thorium_tracepoint_session *tracepoint_session);
void thorium_tracepoint_node_run_loop_test_requests(uint64_t time, struct thorium_tracepoint_session *tracepoint_session);
void thorium_tracepoint_node_run_loop_do_triage(uint64_t time, struct thorium_tracepoint_session *tracepoint_session);

#endif
