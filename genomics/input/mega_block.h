
#ifndef BSAL_MEGA_BLOCK
#define BSAL_MEGA_BLOCK

#include <inttypes.h>

/*
 * An input block.
 */
struct bsal_mega_block {
    int file;
    uint64_t offset;
    uint64_t entries;
    uint64_t entries_from_start;
};

void bsal_mega_block_init(struct bsal_mega_block *self, int file, uint64_t offset, uint64_t entries,
                uint64_t entries_from_start);
void bsal_mega_block_destroy(struct bsal_mega_block *self);

void bsal_mega_block_set_file(struct bsal_mega_block *self, int file);
void bsal_mega_block_print(struct bsal_mega_block *self);
uint64_t bsal_mega_block_get_entries_from_start(struct bsal_mega_block *self);
void bsal_mega_block_set_entries_from_start(struct bsal_mega_block *self,
                uint64_t entries);

uint64_t bsal_mega_block_get_offset(struct bsal_mega_block *self);
uint64_t bsal_mega_block_get_entries(struct bsal_mega_block *self);
int bsal_mega_block_get_file(struct bsal_mega_block *self);

#endif
