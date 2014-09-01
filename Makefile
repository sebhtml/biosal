CC=mpicc
CFLAGS=-O3 -g -I.
CONFIG_FLAGS=
LDFLAGS=-lm -lz
LD=$(CC)

# Run in quiet mode
Q=@

ECHO=echo
MAKE=make
RM=rm

# first target
all:

include engine/thorium/Makefile.mk
include genomics/Makefile.mk
include core/Makefile.mk

LIBRARY_OBJECTS=
LIBRARY_OBJECTS += $(THORIUM_OBJECTS)
LIBRARY_OBJECTS += $(GENOMICS_OBJECTS)
LIBRARY_OBJECTS += $(CORE_OBJECTS)

# include these after the library
include tests/Makefile.mk
include examples/Makefile.mk

APPLICATION_EXECUTABLES=
APPLICATION_OBJECTS=

include performance/Makefile.mk
include applications/Makefile.mk

# generic build rule
%.o: %.c
#$(Q)$(ECHO) " CFLAGS= $(CFLAGS) $(CONFIG_FLAGS)"
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) $(CONFIG_FLAGS) -c $< -o $@

all: $(TEST_EXECUTABLES) $(EXAMPLE_EXECUTABLES) $(APPLICATION_EXECUTABLES)

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) -f $(LIBRARY_OBJECTS)
	$(Q)$(RM) -f $(TEST_LIBRARY_OBJECTS)
	$(Q)$(RM) -f $(TEST_OBJECTS) $(TEST_EXECUTABLES)
	$(Q)$(RM) -f $(EXAMPLE_OBJECTS) $(EXAMPLE_EXECUTABLES)
	$(Q)$(RM) -f $(APPLICATION_OBJECTS) $(APPLICATION_EXECUTABLES)
