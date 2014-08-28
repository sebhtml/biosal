
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

# modules

THORIUM_OBJECTS += engine/thorium/modules/binomial_tree_message.o
THORIUM_OBJECTS += engine/thorium/modules/proxy_message.o
THORIUM_OBJECTS += engine/thorium/modules/action_helpers.o
THORIUM_OBJECTS += engine/thorium/modules/send_helpers.o

#  scheduler system
THORIUM_OBJECTS += engine/thorium/scheduler/migration.o
THORIUM_OBJECTS += engine/thorium/scheduler/scheduler.o
THORIUM_OBJECTS += engine/thorium/scheduler/scheduling_queue.o
THORIUM_OBJECTS += engine/thorium/scheduler/priority_scheduler.o

# transport system
THORIUM_OBJECTS += engine/thorium/transport/transport.o
THORIUM_OBJECTS += engine/thorium/transport/transport_profiler.o

include engine/thorium/transport/*/Makefile.mk


