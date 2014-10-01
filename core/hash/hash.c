
#include "hash.h"

#include "murmur_hash_2_64_a.h"

uint64_t biosal_hash_data_uint64_t(const void *key, int length, unsigned int seed)
{
    return biosal_murmur_hash_2_64_a(key, length, seed);
}
