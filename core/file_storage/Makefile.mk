# file storage

# support for zlib
CONFIG_ZLIB=y

ZLIB_FLAGS-$(CONFIG_ZLIB) += -DCONFIG_ZLIB
ZLIB_LDFLAGS-$(CONFIG_ZLIB) += -lz

CORE_OBJECTS-y=

CORE_OBJECTS-y += core/file_storage/output/buffered_file_writer.o
CORE_OBJECTS-y += core/file_storage/input/buffered_reader.o
CORE_OBJECTS-y += core/file_storage/input/raw_buffered_reader.o
CORE_OBJECTS-y += core/file_storage/directory.o
CORE_OBJECTS-y += core/file_storage/file.o

CORE_OBJECTS-$(CONFIG_ZLIB) += core/file_storage/input/gzip_buffered_reader.o

CONFIG_CFLAGS += $(ZLIB_FLAGS-y)
CONFIG_LDFLAGS += $(ZLIB_LDFLAGS-y)

CORE_OBJECTS += $(CORE_OBJECTS-y)
