
#include "assembly_arc.h"

#include <core/system/packer.h>

#include <stdio.h>

void bsal_assembly_arc_init(struct bsal_assembly_arc *self, int type,
                struct bsal_dna_kmer *source,
                int destination,
                int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
    self->type = type;

    self->destination = destination;

    bsal_dna_kmer_init_copy(&self->source, source, kmer_length, memory, codec);
}

void bsal_assembly_arc_destroy(struct bsal_assembly_arc *self,
                struct bsal_memory_pool *memory)
{
    self->type = -1;
    self->destination = -1;

    bsal_dna_kmer_destroy(&self->source, memory);
}

int bsal_assembly_arc_pack_size(struct bsal_assembly_arc *self, int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_assembly_arc_pack_unpack(self, BSAL_PACKER_OPERATION_DRY_RUN,
                    NULL, kmer_length, NULL, codec);
}

int bsal_assembly_arc_pack(struct bsal_assembly_arc *self, void *buffer,
                int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_assembly_arc_pack_unpack(self, BSAL_PACKER_OPERATION_PACK,
                    buffer, kmer_length, NULL, codec);
}

int bsal_assembly_arc_unpack(struct bsal_assembly_arc *self, void *buffer,
                int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
    return bsal_assembly_arc_pack_unpack(self, BSAL_PACKER_OPERATION_UNPACK,
                    buffer, kmer_length, memory, codec);
}

int bsal_assembly_arc_pack_unpack(struct bsal_assembly_arc *self, int operation,
                void *buffer, int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
    struct bsal_packer packer;
    int bytes;

    bsal_packer_init(&packer, operation, buffer);

    bytes = 0;

    bsal_packer_work(&packer, &self->type, sizeof(self->type));
    bsal_packer_work(&packer, &self->destination, sizeof(self->destination));

    bytes += bsal_packer_worked_bytes(&packer);

    bytes += bsal_dna_kmer_pack_unpack(&self->source, (char *)buffer + bytes, operation,
                    kmer_length, memory, codec);

#if 0
    if (operation == BSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG %d bytes unpacked for kmer\n", bytes);
        bsal_dna_kmer_print(&self->source, kmer_length, codec, memory);

    } else if (operation == BSAL_PACKER_OPERATION_PACK) {

        printf("DEBUG %d bytes packed for kmer\n", bytes);
        bsal_dna_kmer_print(&self->source, kmer_length, codec, memory);
    }
#endif

    bsal_packer_destroy(&packer);

    return bytes;
}

void bsal_assembly_arc_init_empty(struct bsal_assembly_arc *self)
{
    self->type = -1;
    self->destination = -1;

    bsal_dna_kmer_init_empty(&self->source);
}

int bsal_assembly_arc_equals(struct bsal_assembly_arc *self, struct bsal_assembly_arc *arc,
                int kmer_length, struct bsal_dna_codec *codec)
{
    if (self->type != arc->type) {
        return 0;
    }

    if (self->destination != arc->destination) {
        return 0;
    }

    return bsal_dna_kmer_equals(&self->source, &arc->source, kmer_length, codec);
}
