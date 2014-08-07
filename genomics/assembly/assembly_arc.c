
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

void bsal_assembly_arc_init_mock(struct bsal_assembly_arc *self,
                int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
    int type;
    int destination;
    struct bsal_dna_kmer kmer;

    type = BSAL_ARC_TYPE_PARENT;
    destination = BSAL_NUCLEOTIDE_CODE_A;
    bsal_dna_kmer_init_mock(&kmer, kmer_length, codec, memory);

    bsal_assembly_arc_init(self, type, &kmer, destination, kmer_length, memory, codec);

    bsal_dna_kmer_destroy(&kmer, memory);
}

void bsal_assembly_arc_init_copy(struct bsal_assembly_arc *self,
                struct bsal_assembly_arc *arc,
                int kmer_length, struct bsal_memory_pool *memory,
                struct bsal_dna_codec *codec)
{
    int type;
    int destination;
    struct bsal_dna_kmer *source;

    type = arc->type;
    destination = arc->destination;
    source = &arc->source;

    bsal_assembly_arc_init(self, type, source, destination, kmer_length, memory, codec);
}

struct bsal_dna_kmer *bsal_assembly_arc_source(struct bsal_assembly_arc *self)
{
    return &self->source;
}

int bsal_assembly_arc_type(struct bsal_assembly_arc *self)
{
    return self->type;
}

int bsal_assembly_arc_destination(struct bsal_assembly_arc *self)
{
    return self->destination;
}

void bsal_assembly_arc_print(struct bsal_assembly_arc *self, int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool)
{
    printf("object, class: BioSAL/AssemblyArc, type: ");

    if (bsal_assembly_arc_type(self) == BSAL_ARC_TYPE_PARENT) {
        printf("BSAL_ARC_TYPE_PARENT");

    } else if (bsal_assembly_arc_type(self) == BSAL_ARC_TYPE_CHILD) {

        printf("BSAL_ARC_TYPE_CHILD");
    }

    printf(" source: ");
    bsal_dna_kmer_print(bsal_assembly_arc_source(self),
                    kmer_length, codec, pool);

    printf(" destination: %c",
                    bsal_dna_codec_get_nucleotide_from_code(self->destination));

    printf("\n");
}
