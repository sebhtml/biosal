CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

TESTS=test_fifo test_fifo_array test_hash_table_group test_hash_table test_node
EXAMPLES=example_mock example_ring example_reader example_remote_spawn example_synchronize

all: $(EXAMPLES) $(TESTS)

LIBRARY=

# actor engine
LIBRARY += engine/message.o engine/node.o engine/actor.o engine/script.o \
	engine/work.o engine/worker.o engine/worker_pool.o

# data structures
LIBRARY += structures/hash_table.o structures/hash_table_group.o \
	structures/fifo.o structures/fifo_array.o

# hash functions
LIBRARY += hash/murmur_hash_2_64_a.o

# inputs for actors
LIBRARY += input/input_actor.o input/input_proxy.o input/input.o \
           input/input_operations.o input/buffered_reader.o

# data storage
LIBRARY += data/dna_sequence.o

# formats
LIBRARY += formats/fastq_input.o

# examples
EXAMPLE_MOCK=examples/mock/main.o examples/mock/mock.o examples/mock/buddy.o
EXAMPLE_RING=examples/ring/main.o examples/ring/sender.o
EXAMPLE_READER=examples/reader/main.o examples/reader/reader.o
EXAMPLE_REMOTE_SPAWN=examples/remote_spawn/main.o examples/remote_spawn/table.o
EXAMPLE_SYNCHRONIZE=examples/synchronize/main.o examples/synchronize/stream.o

# tests
TEST_FIFO=tests/test.o tests/test_fifo.o
TEST_FIFO_ARRAY=tests/test.o tests/test_fifo_array.o
TEST_HASH_TABLE_GROUP=tests/test.o tests/test_hash_table_group.o
TEST_HASH_TABLE=tests/test.o tests/test_hash_table.o
TEST_NODE=tests/test.o tests/test_node.o

%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(LIBRARY)
	$(Q)$(RM) $(EXAMPLE_RING) $(EXAMPLE_MOCK) $(EXAMPLE_READER) $(EXAMPLE_REMOTE_SPAWN)
	$(Q)$(RM) $(EXAMPLE_SYNCHRONIZE)
	$(Q)$(RM) $(TEST_FIFO) $(TEST_FIFO_ARRAY) $(TEST_HASH_TABLE_GROUP) $(TEST_NODE)
	$(Q)$(RM) $(TEST_HASH_TABLE)
	$(Q)$(RM) $(EXAMPLES) $(TESTS)

ring: example_ring
	mpiexec -n 2 ./example_ring -threads-per-node 8

mock: example_mock
	mpiexec -n 3 ./example_mock -threads-per-node 7

mock1: example_mock
	mpiexec -n 3 ./example_mock -threads-per-node 1

reader: example_reader
	mpiexec -n 2 ./example_reader -threads-per-node 13 -read ~/dropbox/GPIC.1424-1.1371.fastq

synchronize: example_synchronize
	mpiexec -n 3 ./example_synchronize -threads-per-node 9

remote_spawn: example_remote_spawn
	mpiexec -n 6 ./example_remote_spawn -threads-per-node 1,2,3

not_found: example_reader
	mpiexec -n 3 ./example_reader -threads-per-node 7 -read void.fastq

test: tests

tests: $(TESTS)
	./test_fifo_array
	./test_fifo
	./test_hash_table_group
	./test_hash_table
	./test_node

examples: mock mock1 ring reader not_found remote_spawn synchronize

# tests

test_node: $(LIBRARY) $(TEST_NODE)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

test_fifo: $(LIBRARY) $(TEST_FIFO)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

test_hash_table: $(LIBRARY) $(TEST_HASH_TABLE)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

test_hash_table_group: $(LIBRARY) $(TEST_HASH_TABLE_GROUP)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

test_fifo_array: $(LIBRARY) $(TEST_FIFO_ARRAY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

# examples

example_remote_spawn: $(EXAMPLE_REMOTE_SPAWN) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

example_ring: $(EXAMPLE_RING) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

example_mock: $(EXAMPLE_MOCK) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

example_reader: $(EXAMPLE_READER) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

example_synchronize: $(EXAMPLE_SYNCHRONIZE) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@
