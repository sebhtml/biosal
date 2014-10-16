
#ifndef THORIUM_NODE_TRACEPOINTS_H
#define THORIUM_NODE_TRACEPOINTS_H

#include "tracepoints.h"

#include <stdint.h>

struct thorium_tracepoint_session;

void thorium_tracepoint_node_run_loop_print(struct thorium_tracepoint_session *tracepoint_session, uint64_t time);
void thorium_tracepoint_node_run_loop_receive(struct thorium_tracepoint_session *tracepoint_session, uint64_t time);
void thorium_tracepoint_node_run_loop_run(struct thorium_tracepoint_session *tracepoint_session, uint64_t time);
void thorium_tracepoint_node_run_loop_send(struct thorium_tracepoint_session *tracepoint_session, uint64_t time);
void thorium_tracepoint_node_run_loop_pool_work(struct thorium_tracepoint_session *tracepoint_session, uint64_t time);
void thorium_tracepoint_node_run_loop_test_requests(struct thorium_tracepoint_session *tracepoint_session, uint64_t time);
void thorium_tracepoint_node_run_loop_do_triage(struct thorium_tracepoint_session *tracepoint_session, uint64_t time);

#endif
