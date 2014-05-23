CC=mpicc
CFLAGS=-O2 -g -std=c99 -Wall -pedantic -I. -g -Werror=implicit-function-declaration
LD=$(CC)
Q=@
ECHO=echo

all: test_mock

OBJECTS=engine/bsal_message.o engine/bsal_node.o engine/bsal_actor.o engine/bsal_actor_vtable.o
LIBRARY_TEST=test/interface.o test/mock_actor.o

%.o: %.c
	$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

test_mock: $(LIBRARY_TEST) $(OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

clean:
	$(Q)$(ECHO) "  RM"
	$(Q)$(RM) $(LIBRARY_TEST) $(OBJECTS) test_mock
