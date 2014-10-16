Here is a list of tracepoints (-enable-tracepoints provider:event).
To enable all message tracepoints, use -enable-tracepoints message:*.

- message:actor_send
- message:worker_send
- message:worker_send_enqueue
- message:node_send
- message:node_send_system
- message:node_send_dispatch
- message:node_dispatch_message
- message:worker_pool_enqueue
- message:transport_send
- message:transport_receive
- message:node_receive
- message:worker_receive
- message:actor_receive

- actor:receive_enter
- actor:receive_exit

- transport:send
- transport:receive

- node:run_loop_print
- node:run_loop_receive
- node:run_loop_run
- node:run_loop_send
- node:run_loop_pool_work
- node:run_loop_test_requests
- node:run_loop_do_triage
