
# tracepoints object files

TRACEPOINTS_OBJECTS-y=

CONFIG_LTTNG=n
LTTNG_CFLAGS-$(CONFIG_LTTNG)=-DCONFIG_LTTNG
LTTNG_LDFLAGS-$(CONFIG_LTTNG)=-llttng-ust -ldl
CONFIG_CFLAGS += $(LTTNG_CFLAGS-y)
CONFIG_LDFLAGS += $(LTTNG_LDFLAGS-y)

TRACEPOINTS_OBJECTS-y += tracepoints/message_tracepoints.o
TRACEPOINTS_OBJECTS-y += tracepoints/actor_tracepoints.o
TRACEPOINTS_OBJECTS-y += tracepoints/node_tracepoints.o
TRACEPOINTS_OBJECTS-y += tracepoints/tracepoint_session.o

TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += tracepoints/lttng/message.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += tracepoints/lttng/binomial_tree.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += tracepoints/lttng/ring.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += tracepoints/lttng/actor.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += tracepoints/lttng/transport.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += tracepoints/lttng/scheduler.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += tracepoints/lttng/node.o
TRACEPOINTS_OBJECTS-$(CONFIG_LTTNG) += tracepoints/lttng/worker.o

LIBRARY_OBJECTS += $(TRACEPOINTS_OBJECTS-y)


