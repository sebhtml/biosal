CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

# first target
all:

LIBRARY_OBJECTS=

# actor engine
include engine/Makefile.mk
LIBRARY_OBJECTS += $(ENGINE)

# patterns
include patterns/Makefile.mk
LIBRARY_OBJECTS += $(PATTERNS)

# helpers
include helpers/Makefile.mk
LIBRARY_OBJECTS += $(HELPERS)

# kernels for genomics
include kernels/Makefile.mk
LIBRARY_OBJECTS += $(KERNELS)

# system stuff
include system/Makefile.mk
LIBRARY_OBJECTS += $(SYSTEM)

# storage
include storage/Makefile.mk
LIBRARY_OBJECTS += $(STORAGE)

# data structures
include structures/Makefile.mk
LIBRARY_OBJECTS += $(STRUCTURES)

# hash functions
include hash/Makefile.mk
LIBRARY_OBJECTS += $(HASH)

# inputs for actors
include input/Makefile.mk
LIBRARY_OBJECTS += $(INPUT)

# data storage
include data/Makefile.mk
LIBRARY_OBJECTS += $(DATA)

# formats
include formats/Makefile.mk
LIBRARY_OBJECTS += $(FORMATS)

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
