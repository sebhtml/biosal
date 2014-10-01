
#include "assembly_arc.h"

#include <core/system/packer.h>

#include <stdio.h>

void biosal_assembly_arc_init(struct biosal_assembly_arc *self, int type,
                struct biosal_dna_kmer *source,
                int destination,
                int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    self->type = type;

    self->destination = destination;

    biosal_dna_kmer_init_copy(&self->source, source, kmer_length, memory, codec);
}

void biosal_assembly_arc_destroy(struct biosal_assembly_arc *self,
                struct biosal_memory_pool *memory)
{
    self->type = -1;
    self->destination = -1;

    biosal_dna_kmer_destroy(&self->source, memory);
}

int biosal_assembly_arc_pack_size(struct biosal_assembly_arc *self, int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_assembly_arc_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK_SIZE,
                    NULL, kmer_length, NULL, codec);
}

int biosal_assembly_arc_pack(struct biosal_assembly_arc *self, void *buffer,
                int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_assembly_arc_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK,
                    buffer, kmer_length, NULL, codec);
}

int biosal_assembly_arc_unpack(struct biosal_assembly_arc *self, void *buffer,
                int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    return biosal_assembly_arc_pack_unpack(self, BIOSAL_PACKER_OPERATION_UNPACK,
                    buffer, kmer_length, memory, codec);
}

int biosal_assembly_arc_pack_unpack(struct biosal_assembly_arc *self, int operation,
                void *buffer, int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    struct biosal_packer packer;
    int bytes;

    biosal_packer_init(&packer, operation, buffer);

    bytes = 0;

    biosal_packer_process(&packer, &self->type, sizeof(self->type));
    biosal_packer_process(&packer, &self->destination, sizeof(self->destination));

    bytes += biosal_packer_get_byte_count(&packer);

    bytes += biosal_dna_kmer_pack_unpack(&self->source, (char *)buffer + bytes, operation,
                    kmer_length, memory, codec);

#if 0
    if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {
        printf("DEBUG %d bytes unpacked for kmer\n", bytes);
        biosal_dna_kmer_print(&self->source, kmer_length, codec, memory);

    } else if (operation == BIOSAL_PACKER_OPERATION_PACK) {

        printf("DEBUG %d bytes packed for kmer\n", bytes);
        biosal_dna_kmer_print(&self->source, kmer_length, codec, memory);
    }
#endif

    biosal_packer_destroy(&packer);

    return bytes;
}

void biosal_assembly_arc_init_empty(struct biosal_assembly_arc *self)
{
    self->type = -1;
    self->destination = -1;

    biosal_dna_kmer_init_empty(&self->source);
}

int biosal_assembly_arc_equals(struct biosal_assembly_arc *self, struct biosal_assembly_arc *arc,
                int kmer_length, struct biosal_dna_codec *codec)
{
    if (self->type != arc->type) {
        return 0;
    }

    if (self->destination != arc->destination) {
        return 0;
    }

    return biosal_dna_kmer_equals(&self->source, &arc->source, kmer_length, codec);
}

void biosal_assembly_arc_init_mock(struct biosal_assembly_arc *self,
                int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    int type;
    int destination;
    struct biosal_dna_kmer kmer;

    type = BIOSAL_ARC_TYPE_PARENT;
    destination = BIOSAL_NUCLEOTIDE_CODE_A;
    biosal_dna_kmer_init_mock(&kmer, kmer_length, codec, memory);

    biosal_assembly_arc_init(self, type, &kmer, destination, kmer_length, memory, codec);

    biosal_dna_kmer_destroy(&kmer, memory);
}

void biosal_assembly_arc_init_copy(struct biosal_assembly_arc *self,
                struct biosal_assembly_arc *arc,
                int kmer_length, struct biosal_memory_pool *memory,
                struct biosal_dna_codec *codec)
{
    int type;
    int destination;
    struct biosal_dna_kmer *source;

    type = arc->type;
    destination = arc->destination;
    source = &arc->source;

    biosal_assembly_arc_init(self, type, source, destination, kmer_length, memory, codec);
}

struct biosal_dna_kmer *biosal_assembly_arc_source(struct biosal_assembly_arc *self)
{
    return &self->source;
}

int biosal_assembly_arc_type(struct biosal_assembly_arc *self)
{
    return self->type;
}

int biosal_assembly_arc_destination(struct biosal_assembly_arc *self)
{
    return self->destination;
}

void biosal_assembly_arc_print(struct biosal_assembly_arc *self, int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *pool)
{
    printf("object, class: BioSAL/AssemblyArc, type: ");

    if (biosal_assembly_arc_type(self) == BIOSAL_ARC_TYPE_PARENT) {
        printf("BIOSAL_ARC_TYPE_PARENT");

    } else if (biosal_assembly_arc_type(self) == BIOSAL_ARC_TYPE_CHILD) {

        printf("BIOSAL_ARC_TYPE_CHILD");
    }

    printf(" source: ");
    biosal_dna_kmer_print(biosal_assembly_arc_source(self),
                    kmer_length, codec, pool);

    printf(" destination: %c",
                    biosal_dna_codec_get_nucleotide_from_code(self->destination));

    printf("\n");
}


