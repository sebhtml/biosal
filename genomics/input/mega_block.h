
#ifndef BIOSAL_MEGA_BLOCK
#define BIOSAL_MEGA_BLOCK

#include <inttypes.h>

/*
 * An input block.
 */
struct biosal_mega_block {
    int file;
    uint64_t offset;
    uint64_t entries;
    uint64_t entries_from_start;
};

void biosal_mega_block_init(struct biosal_mega_block *self, int file, uint64_t offset, uint64_t entries,
                uint64_t entries_from_start);
void biosal_mega_block_destroy(struct biosal_mega_block *self);

void biosal_mega_block_set_file(struct biosal_mega_block *self, int file);
void biosal_mega_block_print(struct biosal_mega_block *self);
uint64_t biosal_mega_block_get_entries_from_start(struct biosal_mega_block *self);
void biosal_mega_block_set_entries_from_start(struct biosal_mega_block *self,
                uint64_t entries);

uint64_t biosal_mega_block_get_offset(struct biosal_mega_block *self);
uint64_t biosal_mega_block_get_entries(struct biosal_mega_block *self);
int biosal_mega_block_get_file(struct biosal_mega_block *self);

#endif
