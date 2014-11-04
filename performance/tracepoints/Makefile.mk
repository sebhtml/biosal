
# performance object files

TRACEPOINTS_OBJECTS-y=

#CONFIG_LTTNG=y

TRACEPOINTS_OBJECTS-y += performance/tracepoints/message_tracepoints.o
TRACEPOINTS_OBJECTS-y += performance/tracepoints/actor_tracepoints.o
TRACEPOINTS_OBJECTS-y += performance/tracepoints/node_tracepoints.o
TRACEPOINTS_OBJECTS-y += performance/tracepoints/tracepoint_session.o

TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += performance/tracepoints/lttng/message.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += performance/tracepoints/lttng/binomial_tree.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += performance/tracepoints/lttng/ring.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += performance/tracepoints/lttng/actor.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += performance/tracepoints/lttng/transport.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += performance/tracepoints/lttng/scheduler.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += performance/tracepoints/lttng/node.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += performance/tracepoints/lttng/worker.o

LIBRARY_OBJECTS += $(TRACEPOINTS_OBJECTS-y)


