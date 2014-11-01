
#include "node_tracepoints.h"
#include "tracepoint_session.h"

#include <engine/thorium/node.h>

#include <stdint.h>
#include <inttypes.h>

void thorium_tracepoint_node_run_loop_print(uint64_t time, struct thorium_tracepoint_session *tracepoint_session)
{
        /*
    printf("DEBUG tracepoint node:run_loop_print time %" PRIu64 "\n", time);
        */
    thorium_tracepoint_session_set_tracepoint_time(tracepoint_session,
                    THORIUM_TRACEPOINT_node_run_loop_print, time);
}

void thorium_tracepoint_node_run_loop_receive(uint64_t time, struct thorium_tracepoint_session *tracepoint_session)
{
    thorium_tracepoint_session_set_tracepoint_time(tracepoint_session,
                    THORIUM_TRACEPOINT_node_run_loop_receive, time);
}

void thorium_tracepoint_node_run_loop_run(uint64_t time, struct thorium_tracepoint_session *tracepoint_session)
{
    thorium_tracepoint_session_set_tracepoint_time(tracepoint_session,
                    THORIUM_TRACEPOINT_node_run_loop_run, time);
}

void thorium_tracepoint_node_run_loop_send(uint64_t time, struct thorium_tracepoint_session *tracepoint_session)
{
    thorium_tracepoint_session_set_tracepoint_time(tracepoint_session,
                    THORIUM_TRACEPOINT_node_run_loop_send, time);
}

void thorium_tracepoint_node_run_loop_pool_work(uint64_t time, struct thorium_tracepoint_session *tracepoint_session)
{
    thorium_tracepoint_session_set_tracepoint_time(tracepoint_session,
                    THORIUM_TRACEPOINT_node_run_loop_pool_work, time);
}

void thorium_tracepoint_node_run_loop_test_requests(uint64_t time, struct thorium_tracepoint_session *tracepoint_session)
{
    thorium_tracepoint_session_set_tracepoint_time(tracepoint_session,
                    THORIUM_TRACEPOINT_node_run_loop_test_requests, time);
}

void thorium_tracepoint_node_run_loop_do_triage(uint64_t time, struct thorium_tracepoint_session *tracepoint_session)
{
    thorium_tracepoint_session_set_tracepoint_time(tracepoint_session,
                    THORIUM_TRACEPOINT_node_run_loop_do_triage, time);
}

