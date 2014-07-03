CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

# first target
all:

LIBRARY_OBJECTS=

include core/Makefile.mk
LIBRARY_OBJECTS += $(CORE_OBJECTS)

include genomics/Makefile.mk
LIBRARY_OBJECTS += $(GENOMICS)

# generic build rule
%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

# include these after the library
include tests/Makefile.mk
include examples/Makefile.mk
include applications/Makefile.mk

all: $(TEST_EXECUTABLES) $(EXAMPLE_EXECUTABLES) $(APPLICATION_EXECUTABLES)

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(LIBRARY_OBJECTS)
	$(Q)$(RM) $(TEST_OBJECTS) $(TEST_EXECUTABLES)
	$(Q)$(RM) $(EXAMPLE_OBJECTS) $(EXAMPLE_EXECUTABLES)
	$(Q)$(RM) $(APPLICATION_OBJECTS) $(APPLICATION_EXECUTABLES)
