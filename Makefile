CC=mpicc
CFLAGS=-O3 -g -I.
LDFLAGS=-lm
LD=$(CC)

# conditional flags
CONFIG_FLAGS=
CONFIG_LDFLAGS=

# Run in quiet mode
Q=@

ECHO=echo
MAKE=make
RM=rm

APPLICATION_EXECUTABLES=
APPLICATION_OBJECTS=
LIBRARY_OBJECTS=

# first target
all:

include engine/thorium/Makefile.mk
include genomics/Makefile.mk
include core/Makefile.mk

# tracepoints and performance apps.
include performance/Makefile.mk

# include these after the library Makefile.mk files
include tests/Makefile.mk
include examples/Makefile.mk

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
	$(Q)$(RM) -f performance/tracepoints/lttng/*.{o,h,c}
