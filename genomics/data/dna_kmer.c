
#include "dna_kmer.h"

#include "dna_codec.h"

#include <genomics/helpers/dna_helper.h>

#include <core/system/packer.h>
#include <core/system/memory.h>

#include <core/hash/murmur_hash_2_64_a.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include <inttypes.h>

/*
#define BSAL_DNA_SEQUENCE_DEBUG
*/
void bsal_dna_kmer_init(struct bsal_dna_kmer *sequence, char *data,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
    int encoded_length;
    int kmer_length;

    if (data == NULL) {
        sequence->encoded_data = NULL;
        kmer_length = 0;
    } else {

        kmer_length = strlen(data);

        encoded_length = bsal_dna_codec_encoded_length(codec, kmer_length);
        sequence->encoded_data = bsal_memory_pool_allocate(memory, encoded_length);
        bsal_dna_codec_encode(codec, kmer_length, data, sequence->encoded_data);
    }
}

void bsal_dna_kmer_destroy(struct bsal_dna_kmer *sequence, struct bsal_memory_pool *memory)
{
    if (sequence->encoded_data != NULL) {
        bsal_memory_pool_free(memory, sequence->encoded_data);
        sequence->encoded_data = NULL;
    }
}

int bsal_dna_kmer_pack_size(struct bsal_dna_kmer *sequence, int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_dna_kmer_pack_unpack(sequence, NULL, BSAL_PACKER_OPERATION_DRY_RUN,
                    kmer_length, NULL, codec);
}

int bsal_dna_kmer_unpack(struct bsal_dna_kmer *sequence,
                void *buffer, int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
    return bsal_dna_kmer_pack_unpack(sequence, buffer, BSAL_PACKER_OPERATION_UNPACK, kmer_length, memory, codec);
}

int bsal_dna_kmer_pack_store_key(struct bsal_dna_kmer *self,
                void *buffer, int kmer_length, struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
    struct bsal_dna_kmer kmer2;
    int bytes;

    if (bsal_dna_kmer_is_canonical(self, kmer_length, codec)) {
        bytes = bsal_dna_kmer_pack(self, buffer, kmer_length, codec);

    } else {
        bsal_dna_kmer_init_copy(&kmer2, self, kmer_length, memory, codec);
        bsal_dna_kmer_reverse_complement_self(&kmer2, kmer_length, codec, memory);

        bytes = bsal_dna_kmer_pack(&kmer2, buffer, kmer_length, codec);
        bsal_dna_kmer_destroy(&kmer2, memory);
    }

    return bytes;
}

int bsal_dna_kmer_pack(struct bsal_dna_kmer *sequence,
                void *buffer, int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_dna_kmer_pack_unpack(sequence, buffer, BSAL_PACKER_OPERATION_PACK, kmer_length, NULL, codec);
}

int bsal_dna_kmer_pack_unpack(struct bsal_dna_kmer *sequence,
                void *buffer, int operation, int kmer_length,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec)
{
    struct bsal_packer packer;
    int offset;
    int encoded_length;

    bsal_packer_init(&packer, operation, buffer);

    /* don't pack the kmer length...
     */
#if 0
    bsal_packer_work(&packer, kmer_length, sizeof(kmer_length));
#endif

    encoded_length = bsal_dna_codec_encoded_length(codec, kmer_length);

    /* encode in 2 bits instead !
     */
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        if (kmer_length > 0) {
            sequence->encoded_data = bsal_memory_pool_allocate(memory, encoded_length);
        } else {
            sequence->encoded_data = NULL;
        }
    }

    if (kmer_length > 0) {
        bsal_packer_work(&packer, sequence->encoded_data, encoded_length);
    }

    offset = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

    return offset;
}

void bsal_dna_kmer_init_random(struct bsal_dna_kmer *sequence, int kmer_length,
        struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
    char *dna;
    int i;
    int code;

    dna = (char *)bsal_memory_pool_allocate(memory, kmer_length + 1);

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

    bsal_dna_kmer_init(sequence, dna, codec, memory);
    bsal_memory_pool_free(memory, dna);
}

void bsal_dna_kmer_init_mock(struct bsal_dna_kmer *sequence, int kmer_length,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
    char *dna;
    int i;

    dna = (char *)bsal_memory_pool_allocate(memory, kmer_length + 1);

    for (i = 0; i < kmer_length; i++) {
        dna[i] = 'A';
    }

    dna[kmer_length] = '\0';

    bsal_dna_kmer_init(sequence, dna, codec, memory);
    bsal_memory_pool_free(memory, dna);
}

void bsal_dna_kmer_init_copy(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other, int kmer_length,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec)
{
    int encoded_length;

    encoded_length = bsal_dna_codec_encoded_length(codec, kmer_length);
    self->encoded_data = bsal_memory_pool_allocate(memory, encoded_length);
    memcpy(self->encoded_data, other->encoded_data, encoded_length);
}

void bsal_dna_kmer_print(struct bsal_dna_kmer *self, int kmer_length,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
    char *dna_sequence;

    dna_sequence = bsal_memory_pool_allocate(memory, kmer_length + 1);

    bsal_dna_codec_decode(codec, kmer_length, self->encoded_data, dna_sequence);

    printf("KMER length: %d nucleotides, sequence: %s\n", kmer_length,
                   dna_sequence);

    bsal_memory_pool_free(memory, dna_sequence);
    dna_sequence = NULL;
}

uint64_t bsal_dna_kmer_hash(struct bsal_dna_kmer *self, int kmer_length,
                struct bsal_dna_codec *codec)
{
    unsigned int seed;
    /*char *sequence;*/
    int encoded_length;
    uint64_t hash;

    encoded_length = bsal_dna_codec_encoded_length(codec, kmer_length);
    seed = 0xcaa9cfcf;

    /*
     * comment this block
     */
#if 0
    hash = bsal_murmur_hash_2_64_a(self->encoded_data, kmer_length, seed);

    return hash;

    /* Decode sequence because otherwise we'll get a bad performance
     * Update: this is not true. The encoded data is good enough even
     * if it is shorter.
     */
    sequence = b1sal_allocate(kmer_length + 1);
    bsal_dna_kmer_get_sequence(self, sequence);
#endif

    /*hash = bsal_murmur_hash_2_64_a(sequence, kmer_length, seed);*/

    hash = bsal_murmur_hash_2_64_a(self->encoded_data, encoded_length, seed);
/*
    printf("%s %" PRIu64 "\n", sequence, hash);
    b1sal_free(sequence);
*/

    return hash;
}

int bsal_dna_kmer_store_index(struct bsal_dna_kmer *self, int stores, int kmer_length,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
    uint64_t hash;
    int store_index;
    struct bsal_dna_kmer kmer2;

    if (bsal_dna_kmer_is_canonical(self, kmer_length, codec)) {
        hash = bsal_dna_kmer_hash(self, kmer_length, codec);

    } else {
        bsal_dna_kmer_init_copy(&kmer2, self, kmer_length, memory, codec);
        bsal_dna_kmer_reverse_complement_self(&kmer2, kmer_length, codec, memory);

        hash = bsal_dna_kmer_hash(&kmer2, kmer_length, codec);
        bsal_dna_kmer_destroy(&kmer2, memory);
    }

    store_index = hash % stores;

    return store_index;
}

void bsal_dna_kmer_get_sequence(struct bsal_dna_kmer *self, char *sequence, int kmer_length,
                struct bsal_dna_codec *codec)
{
        /*
    printf("get sequence %d\n", kmer_length);
    */
    bsal_dna_codec_decode(codec, kmer_length, self->encoded_data, sequence);
}

/*
 * This function is useless...
 */
int bsal_dna_kmer_length(struct bsal_dna_kmer *self, int kmer_length)
{
    return kmer_length;
}

void bsal_dna_kmer_reverse_complement_self(struct bsal_dna_kmer *self, int kmer_length,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
#ifdef BSAL_DNA_CODEC_HAS_REVERSE_COMPLEMENT_IMPLEMENTATION
    bsal_dna_codec_reverse_complement_in_place(codec, kmer_length, self->encoded_data);

#else
    char *sequence;

    sequence = bsal_memory_pool_allocate(memory, kmer_length + 1);
    bsal_dna_kmer_get_sequence(self, sequence, kmer_length, codec);

#ifdef BSAL_DNA_KMER_DEBUG
    printf("DEBUG %p before %s\n", (void *)self, sequence);
#endif

    bsal_dna_helper_reverse_complement_in_place(sequence);

#ifdef BSAL_DNA_KMER_DEBUG
    printf("DEBUG %p after %s\n", (void *)self, sequence);
#endif

    bsal_dna_kmer_destroy(self, memory);
    bsal_dna_kmer_init(self, sequence, codec, memory);

    bsal_memory_pool_free(memory, sequence);
    sequence = NULL;
#endif
}

int bsal_dna_kmer_is_lower(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other, int kmer_length,
                struct bsal_dna_codec *codec)
{
    int result;

    result = bsal_dna_kmer_compare(self, other, kmer_length, codec);

    if (result < 0) {
        return 1;
    }
    return 0;

}

int bsal_dna_kmer_compare(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other, int kmer_length,
                struct bsal_dna_codec *codec)
{
    int encoded_length;
    encoded_length = bsal_dna_codec_encoded_length(codec, kmer_length);

    return memcmp(self->encoded_data, other->encoded_data, encoded_length);

#if 0
    char *sequence1;
    char *sequence2;
    int result;

    sequence1 = b1sal_allocate(kmer_length + 1);
    sequence2 = b1sal_allocate(kmer_length + 1);

    bsal_dna_kmer_get_sequence(self, sequence1, kmer_length, codec);
    bsal_dna_kmer_get_sequence(other, sequence2, kmer_length, codec);

    result = strcmp(sequence1, sequence2);

    b1sal_free(sequence1);
    b1sal_free(sequence2);

    return result;
#endif
}

int bsal_dna_kmer_is_canonical(struct bsal_dna_kmer *self, int kmer_length,
                struct bsal_dna_codec *codec)
{
    return bsal_dna_codec_is_canonical(codec, kmer_length, self->encoded_data);
}

void bsal_dna_kmer_init_empty(struct bsal_dna_kmer *sequence)
{
    sequence->encoded_data = NULL;
}

int bsal_dna_kmer_equals(struct bsal_dna_kmer *self, struct bsal_dna_kmer *kmer,
                int kmer_length, struct bsal_dna_codec *codec)
{
    int encoded_length;
    int result;

    encoded_length = bsal_dna_codec_encoded_length(codec, kmer_length);

    result = memcmp(self->encoded_data, kmer->encoded_data, encoded_length);

#if 0
    printf("DEBUG bsal_dna_kmer_equals %d %s %s\n", encoded_length,
                    (char *)self->encoded_data,
                    (char *)kmer->encoded_data);
#endif

    if (result == 0) {
        return 1;
    }

    return 0;
}

int bsal_dna_kmer_first_symbol(struct bsal_dna_kmer *self,
                int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_dna_codec_get_nucleotide_code(codec, self->encoded_data, 0);
}

int bsal_dna_kmer_last_symbol(struct bsal_dna_kmer *self,
                int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_dna_codec_get_nucleotide_code(codec, self->encoded_data, kmer_length - 1);
}

void bsal_dna_kmer_init_as_child(struct bsal_dna_kmer *self, struct bsal_dna_kmer *other,
                int code,
                int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{

    bsal_dna_kmer_init_copy(self, other, kmer_length, memory, codec);

    bsal_dna_codec_mutate_as_child(codec, kmer_length, self->encoded_data, code);
}
