
CORE_OBJECTS=

# hot code above
# cold code below
# core stuff

CORE_OBJECTS += core/patterns/manager.o
CORE_OBJECTS += core/patterns/writer_process.o

CORE_OBJECTS += core/hash/murmur_hash_2_64_a.o
CORE_OBJECTS += core/hash/hash.o

# helpers
CORE_OBJECTS += core/helpers/vector_helper.o
CORE_OBJECTS += core/helpers/map_helper.o
CORE_OBJECTS += core/helpers/set_helper.o
CORE_OBJECTS += core/helpers/pair.o
CORE_OBJECTS += core/helpers/statistics.o
CORE_OBJECTS += core/helpers/bitmap.o
CORE_OBJECTS += core/helpers/unit_prefix.o

include core/system/Makefile.mk

include core/file_storage/Makefile.mk

include core/structures/Makefile.mk

LIBRARY_OBJECTS += $(CORE_OBJECTS)

