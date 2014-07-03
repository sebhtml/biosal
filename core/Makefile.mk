
CORE_OBJECTS =

# actor engine
include core/engine/Makefile.mk
CORE_OBJECTS += $(ENGINE)

# patterns
include core/patterns/Makefile.mk
CORE_OBJECTS += $(PATTERNS)

# helpers
include core/helpers/Makefile.mk
CORE_OBJECTS += $(CORE_HELPERS)

# system stuff
include core/system/Makefile.mk
CORE_OBJECTS += $(SYSTEM)

# data structures
include core/structures/Makefile.mk
CORE_OBJECTS += $(STRUCTURES)

# hash functions
include core/hash/Makefile.mk
CORE_OBJECTS += $(HASH)


