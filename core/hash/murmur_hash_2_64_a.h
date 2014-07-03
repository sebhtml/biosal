
#include <stdint.h>

/*
 * MurmurHash64A (MurmurHash 2,  64 bits output,  implementation A)
 *
 * MurmurHash2 is in the public domain and was written by Austin Appleby.
 * \see https://code.google.com/p/smhasher/source/browse/trunk/MurmurHash2.cpp
 *
 * MurmurHash3 would also be interesting.
 * but there are only 32 and 128 versions. The first 64 bits of the 128 bits
 * could be used though.
 * \see https://code.google.com/p/smhasher/source/browse/trunk/MurmurHash3.cpp
 *
 * CityHash64 would also be interesting, but it uses C++ templates.
 * \see https://code.google.com/p/cityhash/source/browse/trunk/src/city.cc
 * \see http://google-opensource.blogspot.com/2011/04/introducing-cityhash.html
 */
uint64_t bsal_murmur_hash_2_64_a(const void *key, int len, unsigned int seed);
