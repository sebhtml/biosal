
LTTng tracepoints
=================

To use LTTng to trace Thorium applications, the compilation
option "CONFIG_LTTNG" is required.


To get a complete list, use the command **lttng list -u**.

To learn how to use LTTng, go to http://lttng.org/docs/

Actor events
------------

- thorium_actor:receive_enter
- thorium_actor:receive_exit

Message delivery path
---------------------

Here is a list of LTTng tracepoints for the delivery path of
messages.

- thorium_message:actor_send
- thorium_message:worker_send
- thorium_message:worker_send_enqueue
- thorium_message:node_send
- thorium_message:node_send_system
- thorium_message:node_send_dispatch
- thorium_message:node_dispatch_message
- thorium_message:worker_pool_enqueue
- thorium_message:transport_send
- thorium_message:transport_receive
- thorium_message:node_receive
- thorium_message:worker_receive
- thorium_message:actor_receive


Other tracepoints (not with LTTng)
===========================

These tracepoints are outdated. LTTng is better.

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
