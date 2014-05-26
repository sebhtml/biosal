CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

all: test_mock

LIBRARY=engine/message.o engine/node.o engine/actor.o engine/actor_vtable.o \
        engine/fifo.o engine/work.o \
        engine/fifo_array.o engine/thread.o

MOCK_EXAMPLE=examples/mock/main.o examples/mock/mock.o examples/mock/buddy.o

TEST_FIFO=test/test.o test/test_fifo.o
TEST_FIFO_ARRAY=test/test.o test/test_fifo_array.o

PRODUCTS=test_mock test_fifo test_fifo_array

%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

test_mock: $(MOCK_EXAMPLE) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(MOCK_EXAMPLE) $(LIBRARY) $(TEST_FIFO) $(TEST_FIFO_ARRAY) $(PRODUCTS)

mock: test_mock
	mpiexec -n 3 ./test_mock

test: test_fifo test_fifo_array
	./test_fifo
	./test_fifo_array

test_fifo: $(LIBRARY) $(TEST_FIFO)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

test_fifo_array: $(LIBRARY) $(TEST_FIFO_ARRAY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@
