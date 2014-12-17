
# transport system
THORIUM_OBJECTS += engine/thorium/transport/transport.o
THORIUM_OBJECTS += engine/thorium/transport/message_multiplexer.o
THORIUM_OBJECTS += engine/thorium/transport/multiplexer_policy.o
THORIUM_OBJECTS += engine/thorium/transport/multiplexed_buffer.o
THORIUM_OBJECTS += engine/thorium/transport/transport_profiler.o
THORIUM_OBJECTS += engine/thorium/transport/decision_maker.o

CONFIG_MPI=y

MPI_CFLAGS-$(CONFIG_MPI) = "-DCONFIG_MPI"
CONFIG_CFLAGS += $(MPI_CFLAGS-y)

include engine/thorium/transport/*/Makefile.mk


