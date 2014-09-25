
THORIUM_OBJECTS=

# main objects for the actor system
THORIUM_OBJECTS += engine/thorium/message.o
THORIUM_OBJECTS += engine/thorium/node.o
THORIUM_OBJECTS += engine/thorium/actor.o
THORIUM_OBJECTS += engine/thorium/script.o
THORIUM_OBJECTS += engine/thorium/worker.o
THORIUM_OBJECTS += engine/thorium/worker_pool.o
THORIUM_OBJECTS += engine/thorium/thorium_engine.o
THORIUM_OBJECTS += engine/thorium/dispatcher.o
THORIUM_OBJECTS += engine/thorium/route.o
THORIUM_OBJECTS += engine/thorium/worker_buffer.o
THORIUM_OBJECTS += engine/thorium/load_profiler.o

# actor modules. These are mostly traits.
THORIUM_OBJECTS += engine/thorium/modules/binomial_tree_message.o
THORIUM_OBJECTS += engine/thorium/modules/proxy_message.o
THORIUM_OBJECTS += engine/thorium/modules/actions.o
THORIUM_OBJECTS += engine/thorium/modules/send_helpers.o
THORIUM_OBJECTS += engine/thorium/modules/active_message_limit.o
THORIUM_OBJECTS += engine/thorium/modules/stop.o

#  scheduler system
THORIUM_OBJECTS += engine/thorium/scheduler/migration.o
THORIUM_OBJECTS += engine/thorium/scheduler/balancer.o
THORIUM_OBJECTS += engine/thorium/scheduler/scheduler.o
THORIUM_OBJECTS += engine/thorium/scheduler/fifo_scheduler.o
THORIUM_OBJECTS += engine/thorium/scheduler/priority_assigner.o

# transport system
THORIUM_OBJECTS += engine/thorium/transport/transport.o
THORIUM_OBJECTS += engine/thorium/transport/message_multiplexer.o
THORIUM_OBJECTS += engine/thorium/transport/multiplexer_policy.o
THORIUM_OBJECTS += engine/thorium/transport/transport_profiler.o

include engine/thorium/transport/*/Makefile.mk


