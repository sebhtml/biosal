
CORE_OBJECTS=

# hot code above
# cold code below
# core stuff

CORE_OBJECTS += core/patterns/manager.o
CORE_OBJECTS += core/patterns/writer_process.o

CORE_OBJECTS += core/hash/murmur_hash_2_64_a.o
CORE_OBJECTS += core/hash/hash.o

# helpers
CORE_OBJECTS += core/helpers/message_helper.o
CORE_OBJECTS += core/helpers/vector_helper.o
CORE_OBJECTS += core/helpers/map_helper.o
CORE_OBJECTS += core/helpers/set_helper.o
CORE_OBJECTS += core/helpers/pair.o
CORE_OBJECTS += core/helpers/statistics.o
CORE_OBJECTS += core/helpers/bitmap.o

# system stuff
CORE_OBJECTS += core/system/spinlock.o

# ticket spinlock
CORE_OBJECTS += core/system/ticket_spinlock.o

CORE_OBJECTS += core/system/command.o
CORE_OBJECTS += core/system/counter.o
CORE_OBJECTS += core/system/debugger.o
CORE_OBJECTS += core/system/packer.o
CORE_OBJECTS += core/system/memory.o
CORE_OBJECTS += core/system/timer.o
CORE_OBJECTS += core/system/tracer.o
CORE_OBJECTS += core/system/memory_pool.o
CORE_OBJECTS += core/system/memory_block.o
CORE_OBJECTS += core/system/atomic.o
CORE_OBJECTS += core/system/thread.o

include core/file_storage/Makefile.mk

include core/structures/Makefile.mk

LIBRARY_OBJECTS += $(CORE_OBJECTS)

