
#ifndef BIOSAL_BITMAP_H
#define BIOSAL_BITMAP_H

#include <stdint.h>

#define BIOSAL_BIT_ZERO 0
#define BIOSAL_BIT_ONE 1

#define BIOSAL_BITS_PER_BYTE 8

int biosal_bitmap_get_bit_uint8_t(uint8_t *self, int index);
void biosal_bitmap_set_bit_value_uint8_t(uint8_t *self, int index, int value);

int biosal_bitmap_get_bit_uint32_t(uint32_t *self, int index);
void biosal_bitmap_set_bit_value_uint32_t(uint32_t *self, int index, int value);

void biosal_bitmap_set_bit_uint32_t(uint32_t *self, int index);
void biosal_bitmap_clear_bit_uint32_t(uint32_t *self, int index);

int biosal_bitmap_get_bit_uint64_t(uint64_t *self, int index);
void biosal_bitmap_set_bit_value_uint64_t(uint64_t *self, int index, int value);

#endif
