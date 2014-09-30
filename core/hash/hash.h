
#ifndef BSAL_HASH_H
#define BSAL_HASH_H

#include <stdint.h>

uint64_t bsal_hash_data_uint64_t(const void *data, int length, unsigned int seed);

#endif
