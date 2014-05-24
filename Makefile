CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

all: test_mock

LIBRARY=engine/bsal_message.o engine/bsal_node.o engine/bsal_actor.o engine/bsal_actor_vtable.o \
        engine/bsal_fifo.o engine/bsal_work.o \
        engine/bsal_fifo_array.o

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

# qemu causes this with -march=native:
# test/interface.c:1:0: error: CPU you selected does not support x86-64 instruction set
#make CFLAGS="-O3 -march=native -g -std=c99 -Wall -pedantic -I. -Werror" -j 7
mock:
	make clean
	make -j 7
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
