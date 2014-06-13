CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

# first target
all:

LIBRARY=

# actor engine
include engine/Makefile.mk
LIBRARY += $(ENGINE)

# patterns
include patterns/Makefile.mk
LIBRARY += $(PATTERNS)

# kernels for genomics
include kernels/Makefile.mk
LIBRARY += $(KERNELS)

# system stuff
include system/Makefile.mk
LIBRARY += $(SYSTEM)

# storage
include storage/Makefile.mk
LIBRARY += $(STORAGE)

# data structures
include structures/Makefile.mk
LIBRARY += $(STRUCTURES)

# hash functions
include hash/Makefile.mk
LIBRARY += $(HASH)

# inputs for actors
include input/Makefile.mk
LIBRARY += $(INPUT)

# data storage
include data/Makefile.mk
LIBRARY += $(DATA)

# formats
include formats/Makefile.mk
LIBRARY += $(FORMATS)

# generic build rule
%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

# include these after the library
include tests/Makefile.mk
include examples/Makefile.mk
include applications/Makefile.mk

all: $(APPLICATIONS) $(EXAMPLES) $(TESTS)

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(LIBRARY)
	$(Q)$(RM) $(EXAMPLE_RING) $(EXAMPLE_MOCK) $(EXAMPLE_READER) $(EXAMPLE_REMOTE_SPAWN)
	$(Q)$(RM) $(EXAMPLE_SYNCHRONIZE) $(EXAMPLE_CONTROLLER) $(EXAMPLE_HELLO_WORLD)
	$(Q)$(RM) $(EXAMPLE_CLONE) $(EXAMPLE_MIGRATION)
	$(Q)$(RM) $(TEST_FIFO) $(TEST_FIFO_ARRAY) $(TEST_HASH_TABLE_GROUP) $(TEST_NODE)
	$(Q)$(RM) $(TEST_HASH_TABLE) $(TEST_VECTOR) $(TEST_DYNAMIC_HASH_TABLE)
	$(Q)$(RM) $(TEST_PACKER) $(TEST_DNA_SEQUENCE)
	$(Q)$(RM) $(EXAMPLES) $(TESTS)
	$(Q)$(RM) $(EXAMPLES) $(TESTS) $(APPLICATIONS) $(APPLICATION_ARGONNITE_OBJECTS)

