
#include "dna_sequence.h"

#include "dna_codec.h"

#include <genomics/helpers/dna_helper.h>

#include <core/system/packer.h>
#include <core/system/memory.h>
#include <core/system/debugger.h>
#include <core/system/memory_pool.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
#define BIOSAL_DNA_SEQUENCE_DEBUG
*/
void biosal_dna_sequence_init(struct biosal_dna_sequence *sequence, char *data,
                struct biosal_dna_codec *codec, struct core_memory_pool *memory)
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

        biosal_dna_helper_normalize(data);

#ifdef BIOSAL_DNA_SEQUENCE_DEBUG
        printf("after normalization %s\n", data);
#endif

        sequence->length_in_nucleotides = strlen(data);

        encoded_length = biosal_dna_codec_encoded_length(codec, sequence->length_in_nucleotides);
        sequence->encoded_data = core_memory_pool_allocate(memory, encoded_length);

        biosal_dna_codec_encode(codec, sequence->length_in_nucleotides, data, sequence->encoded_data);
    }

    sequence->pair = -1;
}

void biosal_dna_sequence_destroy(struct biosal_dna_sequence *sequence, struct core_memory_pool *memory)
{
    if (sequence->encoded_data != NULL) {
        core_memory_pool_free(memory, sequence->encoded_data);
        sequence->encoded_data = NULL;
    }

    sequence->encoded_data = NULL;
    sequence->length_in_nucleotides = 0;
    sequence->pair = -1;
}

int biosal_dna_sequence_pack_size(struct biosal_dna_sequence *sequence, struct biosal_dna_codec *codec)
{
    return biosal_dna_sequence_pack_unpack(sequence, NULL, CORE_PACKER_OPERATION_PACK_SIZE, NULL, codec);
}

int biosal_dna_sequence_unpack(struct biosal_dna_sequence *sequence,
                void *buffer, struct core_memory_pool *memory, struct biosal_dna_codec *codec)
{
    return biosal_dna_sequence_pack_unpack(sequence, buffer, CORE_PACKER_OPERATION_UNPACK, memory, codec);
}

int biosal_dna_sequence_pack(struct biosal_dna_sequence *sequence,
                void *buffer, struct biosal_dna_codec *codec)
{
    return biosal_dna_sequence_pack_unpack(sequence, buffer, CORE_PACKER_OPERATION_PACK, NULL, codec);
}

int biosal_dna_sequence_pack_unpack(struct biosal_dna_sequence *sequence,
                void *buffer, int operation, struct core_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    struct core_packer packer;
    int offset;
    int encoded_length;

#ifdef BIOSAL_DNA_SEQUENCE_DEBUG
    printf("DEBUG ENTRY biosal_dna_sequence_pack_unpack operation %d\n",
                    operation);

    if (operation == CORE_PACKER_OPERATION_PACK) {
        biosal_dna_sequence_print(sequence);
    }
#endif

    core_packer_init(&packer, operation, buffer);

    /*
    core_packer_process(&packer, &sequence->pair, sizeof(sequence->pair));
*/
    core_packer_process(&packer, &sequence->length_in_nucleotides, sizeof(sequence->length_in_nucleotides));

    CORE_DEBUGGER_ASSERT(sequence->length_in_nucleotides > 0);

    encoded_length = biosal_dna_codec_encoded_length(codec, sequence->length_in_nucleotides);

    CORE_DEBUGGER_ASSERT(encoded_length > 0);
    CORE_DEBUGGER_ASSERT(encoded_length < 1000000000);

#ifdef BIOSAL_DNA_SEQUENCE_DEBUG
    if (operation == CORE_PACKER_OPERATION_UNPACK) {
        printf("DEBUG biosal_dna_sequence_pack_unpack unpacking: length %d\n",
                        sequence->length_in_nucleotides);

    } else if (operation == CORE_PACKER_OPERATION_PACK) {

        printf("DEBUG biosal_dna_sequence_pack_unpack packing: length %d\n",
                        sequence->length_in_nucleotides);
    }
#endif

    /* TODO: encode in 2 bits instead !
     */
    if (operation == CORE_PACKER_OPERATION_UNPACK) {

        if (sequence->length_in_nucleotides > 0) {
            sequence->encoded_data = core_memory_pool_allocate(memory, encoded_length);
        } else {
            sequence->encoded_data = NULL;
        }
    }

#ifdef BIOSAL_DNA_SEQUENCE_DEBUG
    if (operation == CORE_PACKER_OPERATION_UNPACK) {
        printf("DEBUG unpacking %d bytes\n", sequence->length_in_nucleotides + 1);
    }
#endif

    if (sequence->length_in_nucleotides > 0) {
        core_packer_process(&packer, sequence->encoded_data, encoded_length);
    }

    offset = core_packer_get_byte_count(&packer);

    core_packer_destroy(&packer);

#ifdef BIOSAL_DNA_SEQUENCE_DEBUG
    printf("DEBUG EXIT biosal_dna_sequence_pack_unpack operation %d offset %d\n",
                    operation, offset);
#endif

    return offset;
}

void biosal_dna_sequence_print(struct biosal_dna_sequence *self, struct biosal_dna_codec *codec,
                struct core_memory_pool *memory)
{
    char *dna_sequence;

    dna_sequence = core_memory_pool_allocate(memory, self->length_in_nucleotides + 1);

    biosal_dna_codec_decode(codec, self->length_in_nucleotides, self->encoded_data, dna_sequence);

    printf("DNA: length %d %s\n", self->length_in_nucleotides, dna_sequence);

    core_memory_pool_free(memory, dna_sequence);
    dna_sequence = NULL;
}

int biosal_dna_sequence_length(struct biosal_dna_sequence *self)
{
    return self->length_in_nucleotides;
}

void biosal_dna_sequence_get_sequence(struct biosal_dna_sequence *self, char *sequence,
                struct biosal_dna_codec *codec)
{
    if (sequence == NULL) {
        return;
    }

    biosal_dna_codec_decode(codec, self->length_in_nucleotides, self->encoded_data, sequence);
}

/* copy other to self
 */
void biosal_dna_sequence_init_copy(struct biosal_dna_sequence *self,
                struct biosal_dna_sequence *other, struct biosal_dna_codec *codec,
                struct core_memory_pool *memory)
{
    char *dna;

    /* need +1 for '\0'
     */
    dna = core_memory_pool_allocate(memory, other->length_in_nucleotides + 1);

    biosal_dna_codec_decode(codec, other->length_in_nucleotides, other->encoded_data,
                    dna);

    biosal_dna_sequence_init(self, dna, codec, memory);

    core_memory_pool_free(memory, dna);
    dna = NULL;
}

void biosal_dna_sequence_init_same_data(struct biosal_dna_sequence *self,
                struct biosal_dna_sequence *other)
{
    self->encoded_data = other->encoded_data;
    self->length_in_nucleotides = other->length_in_nucleotides;
    self->pair = other->pair;
}


