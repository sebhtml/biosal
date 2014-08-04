CC=mpicc
CFLAGS=-O3 -g -I.
LDFLAGS=-lm
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
	$(Q)$(RM) -f $(LIBRARY_OBJECTS)
	$(Q)$(RM) -f $(TEST_LIBRARY_OBJECTS)
	$(Q)$(RM) -f $(TEST_OBJECTS) $(TEST_EXECUTABLES)
	$(Q)$(RM) -f $(EXAMPLE_OBJECTS) $(EXAMPLE_EXECUTABLES)
	$(Q)$(RM) -f $(APPLICATION_OBJECTS) $(APPLICATION_EXECUTABLES)
