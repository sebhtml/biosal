
#include "dna_sequence.h"

#include "dna_codec.h"

#include <genomics/helpers/dna_helper.h>

#include <core/system/packer.h>
#include <core/system/memory.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BSAL_DNA_SEQUENCE_DEBUG
*/
void bsal_dna_sequence_init(struct bsal_dna_sequence *sequence, char *data,
                struct bsal_dna_codec *codec, struct bsal_memory_pool *memory)
{
    int encoded_length;

    /*
     * encode @raw_data in 2-bit format
     * use an allocator provided to allocate memory
     */
    if (data == NULL) {
        sequence->encoded_data = NULL;
        sequence->length_in_nucleotides = 0;
    } else {

        bsal_dna_helper_normalize(data);

#ifdef BSAL_DNA_SEQUENCE_DEBUG
        printf("after normalization %s\n", data);
#endif

        sequence->length_in_nucleotides = strlen(data);

        encoded_length = bsal_dna_codec_encoded_length(codec, sequence->length_in_nucleotides);
        sequence->encoded_data = bsal_memory_pool_allocate(memory, encoded_length);

        bsal_dna_codec_encode(codec, sequence->length_in_nucleotides, data, sequence->encoded_data);
    }

    sequence->pair = -1;
}

void bsal_dna_sequence_destroy(struct bsal_dna_sequence *sequence, struct bsal_memory_pool *memory)
{
    if (sequence->encoded_data != NULL) {
        bsal_memory_pool_free(memory, sequence->encoded_data);
        sequence->encoded_data = NULL;
    }

    sequence->encoded_data = NULL;
    sequence->length_in_nucleotides = 0;
    sequence->pair = -1;
}

int bsal_dna_sequence_pack_size(struct bsal_dna_sequence *sequence, struct bsal_dna_codec *codec)
{
    return bsal_dna_sequence_pack_unpack(sequence, NULL, BSAL_PACKER_OPERATION_DRY_RUN, NULL, codec);
}

int bsal_dna_sequence_unpack(struct bsal_dna_sequence *sequence,
                void *buffer, struct bsal_memory_pool *memory, struct bsal_dna_codec *codec)
{
    return bsal_dna_sequence_pack_unpack(sequence, buffer, BSAL_PACKER_OPERATION_UNPACK, memory, codec);
}

int bsal_dna_sequence_pack(struct bsal_dna_sequence *sequence,
                void *buffer, struct bsal_dna_codec *codec)
{
    return bsal_dna_sequence_pack_unpack(sequence, buffer, BSAL_PACKER_OPERATION_PACK, NULL, codec);
}

int bsal_dna_sequence_pack_unpack(struct bsal_dna_sequence *sequence,
                void *buffer, int operation, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
    struct bsal_packer packer;
    int offset;
    int encoded_length;

#ifdef BSAL_DNA_SEQUENCE_DEBUG
    printf("DEBUG ENTRY bsal_dna_sequence_pack_unpack operation %d\n",
                    operation);

    if (operation == BSAL_PACKER_OPERATION_PACK) {
        bsal_dna_sequence_print(sequence);
    }
#endif

    bsal_packer_init(&packer, operation, buffer);

    /*
    bsal_packer_work(&packer, &sequence->pair, sizeof(sequence->pair));
*/
    bsal_packer_work(&packer, &sequence->length_in_nucleotides, sizeof(sequence->length_in_nucleotides));

    encoded_length = bsal_dna_codec_encoded_length(codec, sequence->length_in_nucleotides);

#ifdef BSAL_DNA_SEQUENCE_DEBUG
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG bsal_dna_sequence_pack_unpack unpacking: length %d\n",
                        sequence->length_in_nucleotides);

    } else if (operation == BSAL_PACKER_OPERATION_PACK) {

        printf("DEBUG bsal_dna_sequence_pack_unpack packing: length %d\n",
                        sequence->length_in_nucleotides);
    }
#endif

    /* TODO: encode in 2 bits instead !
     */
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        if (sequence->length_in_nucleotides > 0) {
            sequence->encoded_data = bsal_memory_pool_allocate(memory, encoded_length);
        } else {
            sequence->encoded_data = NULL;
        }
    }

#ifdef BSAL_DNA_SEQUENCE_DEBUG
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG unpacking %d bytes\n", sequence->length_in_nucleotides + 1);
    }
#endif

    if (sequence->length_in_nucleotides > 0) {
        bsal_packer_work(&packer, sequence->encoded_data, encoded_length);
    }

    offset = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

#ifdef BSAL_DNA_SEQUENCE_DEBUG
    printf("DEBUG EXIT bsal_dna_sequence_pack_unpack operation %d offset %d\n",
                    operation, offset);
#endif

    return offset;
}

void bsal_dna_sequence_print(struct bsal_dna_sequence *self, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *memory)
{
    char *dna_sequence;

    dna_sequence = bsal_memory_pool_allocate(memory, self->length_in_nucleotides + 1);

    bsal_dna_codec_decode(codec, self->length_in_nucleotides, self->encoded_data, dna_sequence);

    printf("DNA: length %d %s\n", self->length_in_nucleotides, dna_sequence);

    bsal_memory_pool_free(memory, dna_sequence);
    dna_sequence = NULL;
}

int bsal_dna_sequence_length(struct bsal_dna_sequence *self)
{
    return self->length_in_nucleotides;
}

void bsal_dna_sequence_get_sequence(struct bsal_dna_sequence *self, char *sequence,
                struct bsal_dna_codec *codec)
{
    if (sequence == NULL) {
        return;
    }

    bsal_dna_codec_decode(codec, self->length_in_nucleotides, self->encoded_data, sequence);
}

/* copy other to self
 */
void bsal_dna_sequence_init_copy(struct bsal_dna_sequence *self,
                struct bsal_dna_sequence *other, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *memory)
{
    char *dna;

    /* need +1 for '\0'
     */
    dna = bsal_memory_pool_allocate(memory, other->length_in_nucleotides + 1);

    bsal_dna_codec_decode(codec, other->length_in_nucleotides, other->encoded_data,
                    dna);

    bsal_dna_sequence_init(self, dna, codec, memory);

    bsal_memory_pool_free(memory, dna);
    dna = NULL;
}

void bsal_dna_sequence_init_same_data(struct bsal_dna_sequence *self,
                struct bsal_dna_sequence *other)
{
    self->encoded_data = other->encoded_data;
    self->length_in_nucleotides = other->length_in_nucleotides;
    self->pair = other->pair;
}


