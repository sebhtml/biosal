
#ifndef BIOSAL_HASH_H
#define BIOSAL_HASH_H

#include <stdint.h>

uint64_t biosal_hash_data_uint64_t(const void *data, int length, unsigned int seed);

#endif
