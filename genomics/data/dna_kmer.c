
#include "dna_kmer.h"

#include "dna_codec.h"

#include <genomics/helpers/dna_helper.h>

#include <core/system/packer.h>
#include <core/system/memory.h>
#include <core/system/debugger.h>

#include <core/hash/hash.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include <inttypes.h>

/*
#define BIOSAL_DNA_SEQUENCE_DEBUG
*/
void biosal_dna_kmer_init(struct biosal_dna_kmer *sequence, char *data,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
    int encoded_length;
    int kmer_length;

    if (data == NULL) {
        sequence->encoded_data = NULL;
        kmer_length = 0;
    } else {

        kmer_length = strlen(data);

        encoded_length = biosal_dna_codec_encoded_length(codec, kmer_length);
        sequence->encoded_data = core_memory_pool_allocate(memory, encoded_length);
        biosal_dna_codec_encode(codec, kmer_length, data, sequence->encoded_data);
    }
}

void biosal_dna_kmer_destroy(struct biosal_dna_kmer *sequence, struct core_memory_pool *memory)
{
    if (sequence->encoded_data != NULL) {
        core_memory_pool_free(memory, sequence->encoded_data);
        sequence->encoded_data = NULL;
    }
}

int biosal_dna_kmer_pack_size(struct biosal_dna_kmer *sequence, int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_dna_kmer_pack_unpack(sequence, NULL, CORE_PACKER_OPERATION_PACK_SIZE,
                    kmer_length, NULL, codec);
}

int biosal_dna_kmer_unpack(struct biosal_dna_kmer *sequence,
                void *buffer, int kmer_length, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    return biosal_dna_kmer_pack_unpack(sequence, buffer, CORE_PACKER_OPERATION_UNPACK, kmer_length, memory, codec);
}

int biosal_dna_kmer_pack_store_key(struct biosal_dna_kmer *self,
                void *buffer, int kmer_length, struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
    struct biosal_dna_kmer kmer2;
    int bytes;

    if (biosal_dna_kmer_is_canonical(self, kmer_length, codec)) {
        bytes = biosal_dna_kmer_pack(self, buffer, kmer_length, codec);

    } else {
        biosal_dna_kmer_init_copy(&kmer2, self, kmer_length, memory, codec);
        biosal_dna_kmer_reverse_complement_self(&kmer2, kmer_length, codec, memory);

        bytes = biosal_dna_kmer_pack(&kmer2, buffer, kmer_length, codec);
        biosal_dna_kmer_destroy(&kmer2, memory);
    }

    return bytes;
}

int biosal_dna_kmer_pack(struct biosal_dna_kmer *sequence,
                void *buffer, int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_dna_kmer_pack_unpack(sequence, buffer, CORE_PACKER_OPERATION_PACK, kmer_length, NULL, codec);
}

int biosal_dna_kmer_pack_unpack(struct biosal_dna_kmer *sequence,
                void *buffer, int operation, int kmer_length,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec)
{
    struct core_packer packer;
    int offset;
    int encoded_length;

    core_packer_init(&packer, operation, buffer);

    /* don't pack the kmer length...
     */
#if 0
    core_packer_process(&packer, kmer_length, sizeof(kmer_length));
#endif

    encoded_length = biosal_dna_codec_encoded_length(codec, kmer_length);

    /* encode in 2 bits instead !
     */
    if (operation == CORE_PACKER_OPERATION_UNPACK) {

        if (kmer_length > 0) {
            sequence->encoded_data = core_memory_pool_allocate(memory, encoded_length);
        } else {
            sequence->encoded_data = NULL;
        }
    }

    if (kmer_length > 0) {
        core_packer_process(&packer, sequence->encoded_data, encoded_length);
    }

    offset = core_packer_get_byte_count(&packer);

    core_packer_destroy(&packer);

    return offset;
}

void biosal_dna_kmer_init_random(struct biosal_dna_kmer *sequence, int kmer_length,
        struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
    char *dna;
    int i;
    int code;

    dna = (char *)core_memory_pool_allocate(memory, kmer_length + 1);

    for (i = 0; i < kmer_length; i++) {
        code = rand() % 4;

        if (code == 0) {
            dna[i] = 'A';
        } else if (code == 1) {
            dna[i] = 'T';
        } else if (code == 2) {
            dna[i] = 'C';
        } else if (code == 3) {
            dna[i] = 'G';
        }
    }

    dna[kmer_length] = '\0';

    biosal_dna_kmer_init(sequence, dna, codec, memory);
    core_memory_pool_free(memory, dna);
}

void biosal_dna_kmer_init_mock(struct biosal_dna_kmer *sequence, int kmer_length,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
    char *dna;
    int i;

    dna = (char *)core_memory_pool_allocate(memory, kmer_length + 1);

    for (i = 0; i < kmer_length; i++) {
        dna[i] = 'A';
    }

    dna[kmer_length] = '\0';

    biosal_dna_kmer_init(sequence, dna, codec, memory);
    core_memory_pool_free(memory, dna);
}

void biosal_dna_kmer_init_copy(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other, int kmer_length,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec)
{
    int encoded_length;

    encoded_length = biosal_dna_codec_encoded_length(codec, kmer_length);
    self->encoded_data = core_memory_pool_allocate(memory, encoded_length);
    core_memory_copy(self->encoded_data, other->encoded_data, encoded_length);
}

void biosal_dna_kmer_print(struct biosal_dna_kmer *self, int kmer_length,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
    char *dna_sequence;

    dna_sequence = core_memory_pool_allocate(memory, kmer_length + 1);

    biosal_dna_codec_decode(codec, kmer_length, self->encoded_data, dna_sequence);

    printf("KMER length: %d nucleotides, sequence: %s hash %" PRIu64 "\n", kmer_length,
                   dna_sequence,
                   biosal_dna_kmer_canonical_hash(self, kmer_length, codec, memory));

    core_memory_pool_free(memory, dna_sequence);
    dna_sequence = NULL;
}

uint64_t biosal_dna_kmer_hash(struct biosal_dna_kmer *self, int kmer_length,
                struct biosal_dna_codec *codec)
{
    unsigned int seed;
    /*char *sequence;*/
    int encoded_length;
    uint64_t hash;

    encoded_length = biosal_dna_codec_encoded_length(codec, kmer_length);
    seed = 0xcaa9cfcf;

    /*
     * comment this block
     */
#if 0
    hash = core_hash_data_uint64_t(self->encoded_data, kmer_length, seed);

    return hash;

    /* Decode sequence because otherwise we'll get a bad performance
     * Update: this is not true. The encoded data is good enough even
     * if it is shorter.
     */
    sequence = b1sal_allocate(kmer_length + 1);
    biosal_dna_kmer_get_sequence(self, sequence);
#endif

    /*hash = core_hash_data_uint64_t(sequence, kmer_length, seed);*/

    hash = core_hash_data_uint64_t(self->encoded_data, encoded_length, seed);
/*
    printf("%s %" PRIu64 "\n", sequence, hash);
    b1sal_free(sequence);
*/

    return hash;
}

int biosal_dna_kmer_store_index(struct biosal_dna_kmer *self, int stores, int kmer_length,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
    uint64_t hash;
    int store_index;

    hash = biosal_dna_kmer_canonical_hash(self, kmer_length, codec, memory);

    store_index = hash % stores;

    return store_index;
}

uint64_t biosal_dna_kmer_canonical_hash(struct biosal_dna_kmer *self, int kmer_length,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
    struct biosal_dna_kmer kmer2;
    uint64_t hash;

    if (biosal_dna_kmer_is_canonical(self, kmer_length, codec)) {
        hash = biosal_dna_kmer_hash(self, kmer_length, codec);

    } else {
        biosal_dna_kmer_init_copy(&kmer2, self, kmer_length, memory, codec);
        biosal_dna_kmer_reverse_complement_self(&kmer2, kmer_length, codec, memory);

        hash = biosal_dna_kmer_hash(&kmer2, kmer_length, codec);
        biosal_dna_kmer_destroy(&kmer2, memory);
    }

    return hash;
}

void biosal_dna_kmer_get_sequence(struct biosal_dna_kmer *self, char *sequence, int kmer_length,
                struct biosal_dna_codec *codec)
{
        /*
    printf("get sequence %d\n", kmer_length);
    */
    biosal_dna_codec_decode(codec, kmer_length, self->encoded_data, sequence);
}

/*
 * This function is useless...
 */
int biosal_dna_kmer_length(struct biosal_dna_kmer *self, int kmer_length)
{
    return kmer_length;
}

void biosal_dna_kmer_reverse_complement_self(struct biosal_dna_kmer *self, int kmer_length,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory)
{
#ifdef BIOSAL_DNA_CODEC_HAS_REVERSE_COMPLEMENT_IMPLEMENTATION
    biosal_dna_codec_reverse_complement_in_place(codec, kmer_length, self->encoded_data);

#else
    char *sequence;

    sequence = core_memory_pool_allocate(memory, kmer_length + 1);
    biosal_dna_kmer_get_sequence(self, sequence, kmer_length, codec);

#ifdef BIOSAL_DNA_KMER_DEBUG
    printf("DEBUG %p before %s\n", (void *)self, sequence);
#endif

    biosal_dna_helper_reverse_complement_in_place(sequence);

#ifdef BIOSAL_DNA_KMER_DEBUG
    printf("DEBUG %p after %s\n", (void *)self, sequence);
#endif

    biosal_dna_kmer_destroy(self, memory);
    biosal_dna_kmer_init(self, sequence, codec, memory);

    core_memory_pool_free(memory, sequence);
    sequence = NULL;
#endif
}

int biosal_dna_kmer_is_lower(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other, int kmer_length,
                struct biosal_dna_codec *codec)
{
    int result;

    result = biosal_dna_kmer_compare(self, other, kmer_length, codec);

    if (result < 0) {
        return 1;
    }
    return 0;

}

int biosal_dna_kmer_compare(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other, int kmer_length,
                struct biosal_dna_codec *codec)
{
    int encoded_length;

    encoded_length = biosal_dna_codec_encoded_length(codec, kmer_length);

    return memcmp(self->encoded_data, other->encoded_data, encoded_length);

#if 0
    char *sequence1;
    char *sequence2;
    int result;

    sequence1 = b1sal_allocate(kmer_length + 1);
    sequence2 = b1sal_allocate(kmer_length + 1);

    biosal_dna_kmer_get_sequence(self, sequence1, kmer_length, codec);
    biosal_dna_kmer_get_sequence(other, sequence2, kmer_length, codec);

    result = strcmp(sequence1, sequence2);

    b1sal_free(sequence1);
    b1sal_free(sequence2);

    return result;
#endif
}

int biosal_dna_kmer_is_canonical(struct biosal_dna_kmer *self, int kmer_length,
                struct biosal_dna_codec *codec)
{
    return biosal_dna_codec_is_canonical(codec, kmer_length, self->encoded_data);
}

void biosal_dna_kmer_init_empty(struct biosal_dna_kmer *sequence)
{
    sequence->encoded_data = NULL;
}

int biosal_dna_kmer_equals(struct biosal_dna_kmer *self, struct biosal_dna_kmer *kmer,
                int kmer_length, struct biosal_dna_codec *codec)
{
    int encoded_length;
    int result;

    encoded_length = biosal_dna_codec_encoded_length(codec, kmer_length);

    result = memcmp(self->encoded_data, kmer->encoded_data, encoded_length);

#if 0
    printf("DEBUG biosal_dna_kmer_equals %d %s %s\n", encoded_length,
                    (char *)self->encoded_data,
                    (char *)kmer->encoded_data);
#endif

    if (result == 0) {
        return 1;
    }

    return 0;
}

int biosal_dna_kmer_first_symbol(struct biosal_dna_kmer *self,
                int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_dna_kmer_get_symbol(self, 0, kmer_length, codec);
}

int biosal_dna_kmer_last_symbol(struct biosal_dna_kmer *self,
                int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_dna_kmer_get_symbol(self, kmer_length - 1, kmer_length,
                    codec);
}

int biosal_dna_kmer_get_symbol(struct biosal_dna_kmer *self, int position,
                int kmer_length, struct biosal_dna_codec *codec)
{
    CORE_DEBUGGER_ASSERT(position >= 0);
    CORE_DEBUGGER_ASSERT(position < kmer_length);

    return biosal_dna_codec_get_nucleotide_code(codec, self->encoded_data, position);
}

void biosal_dna_kmer_init_as_child(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other,
                int code, int kmer_length, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    biosal_dna_kmer_init_copy(self, other, kmer_length, memory, codec);
    biosal_dna_codec_mutate_as_child(codec, kmer_length, self->encoded_data, code);
}

void biosal_dna_kmer_init_as_parent(struct biosal_dna_kmer *self, struct biosal_dna_kmer *other,
                int code, int kmer_length, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    biosal_dna_kmer_init_copy(self, other, kmer_length, memory, codec);
    biosal_dna_codec_mutate_as_parent(codec, kmer_length, self->encoded_data, code);
}
