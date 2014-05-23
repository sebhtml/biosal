CC=mpicc
CFLAGS=-O2 -g -I.
LD=$(CC)
Q=@
ECHO=echo

all: test_mock

OBJECTS=engine/bsal_message.o engine/bsal_node.o engine/bsal_actor.o engine/bsal_actor_vtable.o
LIBRARY_TEST=examples/mock/interface.o examples/mock/mock_actor.o examples/mock/buddy.o

%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

test_mock: $(LIBRARY_TEST) $(OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(LIBRARY_TEST) $(OBJECTS) test_mock

mock:
	./scripts/build-dev.sh
	mpiexec -n 3 ./test_mock
