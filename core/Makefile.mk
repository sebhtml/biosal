
CORE_OBJECTS=

# hot code above
# cold code below
# core stuff

CORE_OBJECTS += core/patterns/manager.o

CORE_OBJECTS += core/hash/murmur_hash_2_64_a.o

# helpers
CORE_OBJECTS += core/helpers/message_helper.o
CORE_OBJECTS += core/helpers/vector_helper.o
CORE_OBJECTS += core/helpers/map_helper.o
CORE_OBJECTS += core/helpers/set_helper.o
CORE_OBJECTS += core/helpers/pair.o
CORE_OBJECTS += core/helpers/statistics.o
CORE_OBJECTS += core/helpers/bitmap.o

# system stuff
CORE_OBJECTS += core/system/lock.o
CORE_OBJECTS += core/system/command.o
CORE_OBJECTS += core/system/counter.o
CORE_OBJECTS += core/system/debugger.o
CORE_OBJECTS += core/system/packer.o
CORE_OBJECTS += core/system/memory.o
CORE_OBJECTS += core/system/timer.o
CORE_OBJECTS += core/system/tracer.o
CORE_OBJECTS += core/system/ticket_lock.o
CORE_OBJECTS += core/system/memory_pool.o
CORE_OBJECTS += core/system/memory_block.o
CORE_OBJECTS += core/system/atomic.o
CORE_OBJECTS += core/system/thread.o

# file storage

CORE_OBJECTS += core/file_storage/output/buffered_file_writer.o
CORE_OBJECTS += core/file_storage/input/buffered_reader.o
CORE_OBJECTS += core/file_storage/input/raw_buffered_reader.o
CORE_OBJECTS += core/file_storage/input/gzip_buffered_reader.o
CORE_OBJECTS += core/file_storage/directory.o
CORE_OBJECTS += core/file_storage/file.o

# structures
CORE_OBJECTS += core/structures/hash_table.o
CORE_OBJECTS += core/structures/hash_table_group.o
CORE_OBJECTS += core/structures/queue.o
CORE_OBJECTS += core/structures/vector.o
CORE_OBJECTS += core/structures/string.o
CORE_OBJECTS += core/structures/hash_table_group_iterator.o
CORE_OBJECTS += core/structures/hash_table_iterator.o
CORE_OBJECTS += core/structures/dynamic_hash_table.o
CORE_OBJECTS += core/structures/dynamic_hash_table_iterator.o
CORE_OBJECTS += core/structures/vector_iterator.o
CORE_OBJECTS += core/structures/map.o
CORE_OBJECTS += core/structures/map_iterator.o
CORE_OBJECTS += core/structures/set.o
CORE_OBJECTS += core/structures/set_iterator.o
CORE_OBJECTS += core/structures/ring.o
CORE_OBJECTS += core/structures/linked_ring.o
CORE_OBJECTS += core/structures/fast_queue.o
CORE_OBJECTS += core/structures/fast_ring.o





