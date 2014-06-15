
#include "dna_kmer_block.h"

#include "dna_kmer.h"

#include <structures/vector_iterator.h>
#include <system/packer.h>

#include <system/debugger.h>

#include <stdlib.h>

/*
#define BSAL_DNA_KMER_BLOCK_DEBUG
*/

void bsal_dna_kmer_block_init(struct bsal_dna_kmer_block *self, int kmer_length, int source_index)
{
    self->source_index = source_index;
    self->kmer_length = kmer_length;
    bsal_vector_init(&self->kmers, sizeof(struct bsal_dna_kmer));
}

void bsal_dna_kmer_block_destroy(struct bsal_dna_kmer_block *self)
{
    struct bsal_vector_iterator iterator;
    struct bsal_dna_kmer *kmer;

    /* destroy kmers
     */
    bsal_vector_iterator_init(&iterator, &self->kmers);

    while (bsal_vector_iterator_has_next(&iterator)) {

        bsal_vector_iterator_next(&iterator, (void **)&kmer);

        bsal_dna_kmer_destroy(kmer);
    }

    bsal_vector_iterator_destroy(&iterator);
    self->source_index = -1;
    self->kmer_length = -1;
    bsal_vector_destroy(&self->kmers);
}

void bsal_dna_kmer_block_add_kmer(struct bsal_dna_kmer_block *self, struct bsal_dna_kmer *kmer)
{
    struct bsal_dna_kmer copy;

    bsal_dna_kmer_init_copy(&copy, kmer);

    bsal_vector_push_back(&self->kmers, &copy);
}

int bsal_dna_kmer_block_pack_size(struct bsal_dna_kmer_block *self)
{
    return bsal_dna_kmer_block_pack_unpack(self, NULL, BSAL_PACKER_OPERATION_DRY_RUN);
}

int bsal_dna_kmer_block_pack(struct bsal_dna_kmer_block *self, void *buffer)
{
    return bsal_dna_kmer_block_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_PACK);
}

int bsal_dna_kmer_block_unpack(struct bsal_dna_kmer_block *self, void *buffer)
{

#ifdef BSAL_DNA_KMER_BLOCK_DEBUG
    BSAL_DEBUG_MARKER("unpacking block");
#endif

    return bsal_dna_kmer_block_pack_unpack(self, buffer, BSAL_PACKER_OPERATION_UNPACK);
}

int bsal_dna_kmer_block_pack_unpack(struct bsal_dna_kmer_block *self, void *buffer,
                int operation)
{
    struct bsal_packer packer;
    int offset;
    int elements;
    int i = 0;
    struct bsal_dna_kmer *kmer;
    struct bsal_dna_kmer new_kmer;

    bsal_packer_init(&packer, operation, buffer);

#ifdef BSAL_DNA_KMER_BLOCK_DEBUG
    BSAL_DEBUG_MARKER("pack unpack 1");
#endif

    bsal_packer_work(&packer, &self->kmer_length, sizeof(self->kmer_length));
    bsal_packer_work(&packer, &self->source_index, sizeof(self->source_index));

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        bsal_dna_kmer_block_init(self, self->kmer_length, self->source_index);
    } else {
        elements = bsal_vector_size(&self->kmers);
    }

#ifdef BSAL_DNA_KMER_BLOCK_DEBUG
    BSAL_DEBUG_MARKER("pack unpack 2");
#endif

    bsal_packer_work(&packer, &elements, sizeof(elements));

#ifdef BSAL_DNA_KMER_BLOCK_DEBUG
    printf("DEBUG kmer_length %d source_index %d elements %d\n", self->kmer_length,
                    self->source_index, elements);
#endif

    offset = bsal_packer_worked_bytes(&packer);
    bsal_packer_destroy(&packer);

    /* do the rest manually
     */

#ifdef BSAL_DNA_KMER_BLOCK_DEBUG
    BSAL_DEBUG_MARKER("pack unpack 3");
#endif

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        for (i = 0; i < elements; i++) {
            offset += bsal_dna_kmer_pack_unpack(&new_kmer, (char *)buffer + offset, operation);

            bsal_dna_kmer_block_add_kmer(self, &new_kmer);
        }
    } else {
        for (i = 0; i < elements; i++) {
            kmer = (struct bsal_dna_kmer *)bsal_vector_at(&self->kmers, i);

            offset += bsal_dna_kmer_pack_unpack(kmer, (char *)buffer + offset, operation);
        }
    }

#ifdef BSAL_DNA_KMER_BLOCK_DEBUG
    BSAL_DEBUG_MARKER("pack unpack EXIT");
#endif

    return offset;
}

int bsal_dna_kmer_block_source_index(struct bsal_dna_kmer_block *self)
{
    return self->source_index;
}

struct bsal_vector *bsal_dna_kmer_block_kmers(struct bsal_dna_kmer_block *self)
{
    return &self->kmers;
}
