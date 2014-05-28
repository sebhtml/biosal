CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

TESTS=test_fifo test_fifo_array test_hash_table_group test_hash_table
EXAMPLES=test_mock test_ring

all: $(EXAMPLES) $(TESTS)

LIBRARY=engine/message.o engine/node.o engine/actor.o engine/actor_vtable.o \
	engine/work.o engine/worker_thread.o engine/worker_pool.o \
	structures/hash_table.o structures/hash_table_group.o \
	structures/fifo.o structures/fifo_array.o

MOCK_EXAMPLE=examples/mock/main.o examples/mock/mock.o examples/mock/buddy.o
RING_EXAMPLE=examples/ring/main.o examples/ring/sender.o

TEST_FIFO=test/test.o test/test_fifo.o
TEST_FIFO_ARRAY=test/test.o test/test_fifo_array.o
TEST_HASH_TABLE_GROUP=test/test.o test/test_hash_table_group.o
TEST_HASH_TABLE=test/test.o test/test_hash_table.o

%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(LIBRARY)
	$(Q)$(RM) $(RING_EXAMPLE) $(MOCK_EXAMPLE)
	$(Q)$(RM) $(TEST_FIFO) $(TEST_FIFO_ARRAY) $(TEST_HASH_TABLE_GROUP)
	$(Q)$(RM) $(TEST_HASH_TABLE)
	$(Q)$(RM) $(EXAMPLES) $(TESTS)

ring: test_ring
	mpiexec -n 2 ./test_ring 8

mock: test_mock
	mpiexec -n 3 ./test_mock

mock1: test_mock
	mpiexec -n 3 ./test_mock 1

test: $(TESTS)
	./test_fifo_array
	./test_fifo
	./test_hash_table_group
	./test_hash_table

run: mock mock1 ring

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
