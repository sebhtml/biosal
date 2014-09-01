
# Define the macro for the compiler
CONFIG_PAMI-$(CONFIG_PAMI)=-DCONFIG_PAMI
MACRO_PAMI_CONFIG=$(CONFIG_PAMI-y)
CONFIG_FLAGS+=$(MACRO_PAMI_CONFIG)

THORIUM_OBJECTS-$(CONFIG_PAMI) += engine/thorium/transport/pami/pami_transport.o
THORIUM_OBJECTS += $(THORIUM_OBJECTS-y)

