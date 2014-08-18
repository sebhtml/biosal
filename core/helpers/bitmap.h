
#ifndef BSAL_BITMAP_H
#define BSAL_BITMAP_H

#include <stdint.h>

#define BSAL_BIT_ZERO 0
#define BSAL_BIT_ONE 1

#define BSAL_BITS_PER_BYTE 8

int bsal_bitmap_get_bit_uint8_t(uint8_t *self, int index);
void bsal_bitmap_set_bit_value_uint8_t(uint8_t *self, int index, int value);

int bsal_bitmap_get_bit_uint32_t(uint32_t *self, int index);
void bsal_bitmap_set_bit_value_uint32_t(uint32_t *self, int index, int value);

void bsal_bitmap_set_bit_uint32_t(uint32_t *self, int index);
void bsal_bitmap_clear_bit_uint32_t(uint32_t *self, int index);

int bsal_bitmap_get_bit_uint64_t(uint64_t *self, int index);
void bsal_bitmap_set_bit_value_uint64_t(uint64_t *self, int index, int value);

#endif
