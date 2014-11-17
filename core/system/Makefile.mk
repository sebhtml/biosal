
CONFIG_CLOCK_GETTIME=y
CLOCK_GETTIME_CFLAGS-$(CONFIG_CLOCK_GETTIME)=-DCONFIG_CLOCK_GETTIME
CLOCK_GETTIME_LDFLAGS-$(CONFIG_CLOCK_GETTIME)=-lrt
CONFIG_CFLAGS+=$(CLOCK_GETTIME_CFLAGS-y)
CONFIG_LDFLAGS+=$(CLOCK_GETTIME_LDFLAGS-y)

# Instruction to use jemalloc on Blue Gene/Q
# https://github.com/jeffhammond/HPCInfo/wiki/Malloc#Building_jemalloc_on_Blue_GeneQ
CONFIG_JEMALLOC=n
CONFIG_JEMALLOC_ROOT=$(HOME)/MALLOC/jemalloc-install
CFLAGS_JEMALLOC-$(CONFIG_JEMALLOC)=-DCONFIG_JEMALLOC -I$(CONFIG_JEMALLOC_ROOT)/include
LDFLAGS_JEMALLOC-$(CONFIG_JEMALLOC)=$(CONFIG_JEMALLOC_ROOT)/lib/libjemalloc.a
CONFIG_CFLAGS+=$(CFLAGS_JEMALLOC-y)
CONFIG_LDFLAGS+=$(LDFLAGS_JEMALLOC-y)

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

