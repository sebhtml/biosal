
EXAMPLE_EXECUTABLES=examples/example_mock examples/example_ring examples/example_reader examples/example_remote_spawn \
	examples/example_synchronize examples/example_controller examples/example_hello_world examples/example_systolic \
	examples/example_clone examples/example_migration \
	examples/example_hello_world_acq
EXAMPLE_OBJECTS=

# examples
EXAMPLE_MOCK=examples/mock/main.o examples/mock/mock.o examples/mock/buddy.o
EXAMPLE_OBJECTS+=$(EXAMPLE_MOCK)
EXAMPLE_RING=examples/ring/main.o examples/ring/sender.o examples/ring/ring.o
EXAMPLE_OBJECTS+=$(EXAMPLE_RING)
EXAMPLE_READER=examples/reader/main.o examples/reader/reader.o
EXAMPLE_OBJECTS+=$(EXAMPLE_READER)
EXAMPLE_REMOTE_SPAWN=examples/remote_spawn/main.o examples/remote_spawn/table.o
EXAMPLE_OBJECTS+=$(EXAMPLE_REMOTE_SPAWN)
EXAMPLE_SYNCHRONIZE=examples/synchronize/main.o examples/synchronize/stream.o
EXAMPLE_OBJECTS+=$(EXAMPLE_SYNCHRONIZE)
EXAMPLE_CONTROLLER=examples/controller/main.o examples/controller/root.o
EXAMPLE_OBJECTS+=$(EXAMPLE_CONTROLLER)
EXAMPLE_HELLO_WORLD=examples/hello_world/main.o examples/hello_world/hello.o
EXAMPLE_OBJECTS+=$(EXAMPLE_HELLO_WORLD)
EXAMPLE_HELLO_WORLD_ACQ=examples/hello_acquaintances/main.o examples/hello_acquaintances/hello_acq.o
EXAMPLE_OBJECTS+=$(EXAMPLE_HELLO_WORLD_ACQ)
EXAMPLE_SYSTOLIC=examples/systolic/main.o examples/systolic/systolic.o
EXAMPLE_OBJECTS+=$(EXAMPLE_SYSTOLIC)
EXAMPLE_CLONE=examples/clone/main.o examples/clone/process.o
EXAMPLE_OBJECTS+=$(EXAMPLE_CLONE)
EXAMPLE_MIGRATION=examples/migration/main.o examples/migration/frame.o
EXAMPLE_OBJECTS+=$(EXAMPLE_MIGRATION)

# examples

examples/example_remote_spawn: $(EXAMPLE_REMOTE_SPAWN) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_ring: $(EXAMPLE_RING) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_mock: $(EXAMPLE_MOCK) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_reader: $(EXAMPLE_READER) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_synchronize: $(EXAMPLE_SYNCHRONIZE) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_controller: $(EXAMPLE_CONTROLLER) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_hello_world: $(EXAMPLE_HELLO_WORLD) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_hello_world_acq: $(EXAMPLE_HELLO_WORLD_ACQ) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_systolic: $(EXAMPLE_SYSTOLIC) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_clone: $(EXAMPLE_CLONE) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

examples/example_migration: $(EXAMPLE_MIGRATION) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)

ring: examples/example_ring
	mpiexec -n 2 $< -threads-per-node 8

mock: examples/example_mock
	mpiexec -n 3 $< -threads-per-node 7

mock1: examples/example_mock
	mpiexec -n 3 $< -print-load -threads-per-node 1

reader: examples/example_reader
	mpiexec -n 2 $< -threads-per-node 13 -read ~/dropbox/GPIC.1424-1.1371.fastq

synchronize: examples/example_synchronize
	mpiexec -n 3 $< -threads-per-node 9

controller: examples/example_controller
	mpiexec -n 3 $< -threads-per-node 9 ~/dropbox/GPIC.1424-1.1371.fastq

mini: examples/example_controller
	mpiexec -n 3 $< -threads-per-node 9 ~/mini.fastq

void_controller: examples/example_controller
	mpiexec -n 3 $< -threads-per-node 9 void.fastq

remote_spawn: examples/example_remote_spawn
	mpiexec -n 6 $< -threads-per-node 1,2,3

not_found: examples/example_reader
	mpiexec -n 3 $< -threads-per-node 7 -read void.fastq

hello_world: examples/example_hello_world
	mpiexec -n 3 $< -threads-per-node 5

systolic: examples/example_systolic
	mpiexec -n 3 $< -threads-per-node 5

clone: examples/example_clone
	mpiexec -n 3 $< -threads-per-node 5

migration: examples/example_migration
	mpiexec -n 3 $< -threads-per-node 5

