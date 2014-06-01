CC=mpicc
CFLAGS=-O2 -g -I.
#CFLAGS=-g -I.
LD=$(CC)
Q=@
ECHO=echo

TESTS=test_fifo test_fifo_array test_hash_table_group test_hash_table
EXAMPLES=test_mock test_ring test_reader

all: $(EXAMPLES) $(TESTS)

LIBRARY=

# actor engine
LIBRARY += engine/message.o engine/node.o engine/actor.o engine/actor_vtable.o \
	engine/work.o engine/worker.o engine/worker_pool.o

# data structures
LIBRARY += structures/hash_table.o structures/hash_table_group.o \
	structures/fifo.o structures/fifo_array.o

# hash functions
LIBRARY += hash/murmur_hash_2_64_a.o

# inputs for actors
LIBRARY += input/input_actor.o input/input_proxy.o input/input.o \
           input/input_vtable.o input/buffered_reader.o

# data storage
LIBRARY += data/dna_sequence.o

# formats
LIBRARY += formats/fastq_input.o

# examples
MOCK_EXAMPLE=examples/mock/main.o examples/mock/mock.o examples/mock/buddy.o
RING_EXAMPLE=examples/ring/main.o examples/ring/sender.o
READER_EXAMPLE=examples/reader/main.o examples/reader/reader.o

# tests
TEST_FIFO=tests/test.o tests/test_fifo.o
TEST_FIFO_ARRAY=tests/test.o tests/test_fifo_array.o
TEST_HASH_TABLE_GROUP=tests/test.o tests/test_hash_table_group.o
TEST_HASH_TABLE=tests/test.o tests/test_hash_table.o

%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(LIBRARY)
	$(Q)$(RM) $(RING_EXAMPLE) $(MOCK_EXAMPLE) $(READER_EXAMPLE)
	$(Q)$(RM) $(TEST_FIFO) $(TEST_FIFO_ARRAY) $(TEST_HASH_TABLE_GROUP)
	$(Q)$(RM) $(TEST_HASH_TABLE)
	$(Q)$(RM) $(EXAMPLES) $(TESTS)

ring: test_ring
	mpiexec -n 2 ./test_ring -threads-per-node 8

mock: test_mock
	mpiexec -n 3 ./test_mock -threads-per-node 7

mock1: test_mock
	mpiexec -n 3 ./test_mock -threads-per-node 1

reader: test_reader
	mpiexec -n 2 ./test_reader -threads-per-node 13 -read ~/dropbox/GPIC.1424-1.1371.fastq

not_found: test_reader
	mpiexec -n 3 ./test_reader -threads-per-node 7 -read void.fastq

test: $(TESTS)
	./test_fifo_array
	./test_fifo
	./test_hash_table_group
	./test_hash_table

run: mock mock1 ring reader not_found

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

test_ring: $(RING_EXAMPLE) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

test_mock: $(MOCK_EXAMPLE) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

test_reader: $(READER_EXAMPLE) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@
