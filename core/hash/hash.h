
#ifndef CORE_HASH_H
#define CORE_HASH_H

#include <stdint.h>

uint64_t core_hash_data_uint64_t(const void *data, int length, unsigned int seed);

#endif
