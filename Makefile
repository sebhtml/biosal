CC=mpicc
CFLAGS=-O2 -std=c89 -Wall -pedantic -I.
LD=$(CC)
Q=
ECHO=echo

all: test_mock

OBJECTS=engine/bsal_message.o engine/bsal_node.o engine/bsal_actor.o
LIBRARY_TEST=test/interface.o test/mock_actor.o

%.o: %.c
#$(Q)$(ECHO) "  CC $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@
	
test_mock: $(LIBRARY_TEST) $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	$(RM) $(LIBRARY_TEST) $(OBJECTS)
