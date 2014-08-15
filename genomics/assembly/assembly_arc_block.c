
#include "assembly_arc_block.h"

#include <core/system/packer.h>

void bsal_assembly_arc_block_init(struct bsal_assembly_arc_block *self, struct bsal_memory_pool *pool,
                int kmer_length, struct bsal_dna_codec *codec)
{
    int key_length;
    struct bsal_assembly_arc arc;

    bsal_assembly_arc_init_mock(&arc, kmer_length, pool, codec);

    key_length = bsal_assembly_arc_pack_size(&arc, kmer_length, codec);

    bsal_assembly_arc_destroy(&arc, pool);

    bsal_set_init(&self->set, key_length);

    bsal_vector_init(&self->arcs, sizeof(struct bsal_assembly_arc));

    /*
     * Set the memory pool too.
     * The main use case is to generate stuff to transport between actors so
     * ephemeral memory pool is going to be used.
     */
    bsal_set_set_memory_pool(&self->set, pool);
    bsal_vector_set_memory_pool(&self->arcs, pool);
}

void bsal_assembly_arc_block_destroy(struct bsal_assembly_arc_block *self, struct bsal_memory_pool *pool)
{
    int i;
    int size;
    struct bsal_assembly_arc *arc;

    bsal_set_destroy(&self->set);

    size = bsal_vector_size(&self->arcs);

    /*
     * Destroy arcs.
     */
    for (i = 0; i < size; i++) {

        arc = bsal_vector_at(&self->arcs, i);

        bsal_assembly_arc_destroy(arc, pool);
    }

    bsal_vector_destroy(&self->arcs);
}

void bsal_assembly_arc_block_add_arc(struct bsal_assembly_arc_block *self, struct bsal_assembly_arc *arc,
                int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool)
{
    int key_length;
    void *buffer;
    int found;
    struct bsal_assembly_arc copy;

    key_length = bsal_assembly_arc_pack_size(arc, kmer_length, codec);
    buffer = bsal_memory_pool_allocate(pool, key_length);

    /*
     * Generate a key to verify if this arc is already in the block.
     * Duplicates are not required anyway.
     */
    bsal_assembly_arc_pack(arc, buffer, kmer_length, codec);

    found = bsal_set_find(&self->set, buffer);

    bsal_memory_pool_free(pool, buffer);

    /*
     * Don't append it if the arc is there already
     */
    if (found) {
        return;
    }

    bsal_assembly_arc_init_copy(&copy, arc, kmer_length, pool, codec);

    bsal_vector_push_back(&self->arcs, &copy);
}

int bsal_assembly_arc_block_pack_size(struct bsal_assembly_arc_block *self, int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_assembly_arc_block_pack_unpack(self, BSAL_PACKER_OPERATION_DRY_RUN,
                    NULL, kmer_length, codec, NULL);
}

int bsal_assembly_arc_block_pack(struct bsal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_assembly_arc_block_pack_unpack(self, BSAL_PACKER_OPERATION_PACK,
                    buffer, kmer_length, codec, NULL);
}

int bsal_assembly_arc_block_unpack(struct bsal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool)
{
    return bsal_assembly_arc_block_pack_unpack(self, BSAL_PACKER_OPERATION_UNPACK,
                    buffer, kmer_length, codec, pool);
}

int bsal_assembly_arc_block_pack_unpack(struct bsal_assembly_arc_block *self, int operation,
                void *buffer, int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool)
{
    int size;
    struct bsal_packer packer;
    int bytes;
    int i;
    struct bsal_assembly_arc new_arc;
    struct bsal_assembly_arc *arc;

    bytes = 0;
    bsal_packer_init(&packer, operation, buffer);

    size = bsal_vector_size(&self->arcs);
    bsal_packer_work(&packer, &size, sizeof(size));

    bytes = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

    for (i = 0; i < size; i++) {

        if (operation == BSAL_PACKER_OPERATION_UNPACK) {

            bsal_assembly_arc_init_empty(&new_arc);

            bytes += bsal_assembly_arc_pack_unpack(&new_arc, operation,
                        (char *)buffer + bytes,
                        kmer_length, pool, codec);

            bsal_assembly_arc_block_add_arc(self, &new_arc, kmer_length, codec, pool);

            bsal_assembly_arc_destroy(&new_arc, pool);

        } else {

            arc = bsal_vector_at(&self->arcs, i);

            bytes += bsal_assembly_arc_pack_unpack(arc, operation,
                        (char *)buffer + bytes,
                        kmer_length, pool, codec);

        }
    }

    return bytes;
}

struct bsal_vector *bsal_assembly_arc_block_get_arcs(struct bsal_assembly_arc_block *self)
{
    return &self->arcs;
}

void bsal_assembly_arc_block_reserve(struct bsal_assembly_arc_block *self, int size)
{
    bsal_vector_reserve(&self->arcs, size);
}
