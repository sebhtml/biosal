CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

PRODUCTS=test_mock test_fifo test_fifo_array test_ring

all: $(PRODUCTS)

LIBRARY=engine/message.o engine/node.o engine/actor.o engine/actor_vtable.o \
        engine/fifo.o engine/work.o \
        engine/fifo_array.o engine/thread.o

MOCK_EXAMPLE=examples/mock/main.o examples/mock/mock.o examples/mock/buddy.o
RING_EXAMPLE=examples/ring/main.o examples/ring/sender.o

TEST_FIFO=test/test.o test/test_fifo.o
TEST_FIFO_ARRAY=test/test.o test/test_fifo_array.o

%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(LIBRARY)
	$(Q)$(RM) $(RING_EXAMPLE) $(MOCK_EXAMPLE)
	$(Q)$(RM) $(TEST_FIFO) $(TEST_FIFO_ARRAY)
	$(Q)$(RM) $(PRODUCTS)

ring: test_ring
	mpiexec -n 2 ./test_ring

mock: test_mock
	mpiexec -n 3 ./test_mock

mock1: test_mock
	mpiexec -n 3 ./test_mock 1

test: test_fifo test_fifo_array
	./test_fifo
	./test_fifo_array

test_fifo: $(LIBRARY) $(TEST_FIFO)
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
