
# transport system
THORIUM_OBJECTS += engine/thorium/transport/transport.o
THORIUM_OBJECTS += engine/thorium/transport/message_multiplexer.o
THORIUM_OBJECTS += engine/thorium/transport/multiplexer_policy.o
THORIUM_OBJECTS += engine/thorium/transport/transport_profiler.o

include engine/thorium/transport/*/Makefile.mk


