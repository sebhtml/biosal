
#include "hash.h"

#include "murmur_hash_2_64_a.h"

uint64_t bsal_hash_data_uint64_t(const void *key, int length, unsigned int seed)
{
    return bsal_murmur_hash_2_64_a(key, length, seed);
}
