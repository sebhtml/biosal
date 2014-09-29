
#include "assembly_arc_block.h"

#include <core/system/packer.h>
#include <core/system/debugger.h>

void bsal_assembly_arc_block_init(struct bsal_assembly_arc_block *self, struct bsal_memory_pool *pool,
                int kmer_length, struct bsal_dna_codec *codec)
{
    int key_length;
    struct bsal_assembly_arc arc;

    BSAL_DEBUGGER_LEAK_DETECTION_BEGIN(pool, arc_block_init);

    bsal_assembly_arc_init_mock(&arc, kmer_length, pool, codec);

    key_length = bsal_assembly_arc_pack_size(&arc, kmer_length, codec);

    bsal_assembly_arc_destroy(&arc, pool);

    /*
     * Set the memory pool too.
     * The main use case is to generate stuff to transport between actors so
     * ephemeral memory pool is going to be used.
     */

    bsal_set_init(&self->set, key_length);
    bsal_set_set_memory_pool(&self->set, pool);

    bsal_vector_init(&self->arcs, sizeof(struct bsal_assembly_arc));
    bsal_vector_set_memory_pool(&self->arcs, pool);

    self->enable_redundancy_check = 0;

    BSAL_DEBUGGER_LEAK_DETECTION_END(pool, arc_block_init);
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

        BSAL_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(pool);
    }

    bsal_vector_destroy(&self->arcs);
}

void bsal_assembly_arc_block_add_arc(struct bsal_assembly_arc_block *self,
                int type, struct bsal_dna_kmer *source,
                int destination,
                int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool)
{
    int key_length;
    void *buffer;
    int found;
    struct bsal_assembly_arc *arc;
    int size;

    size = bsal_vector_size(&self->arcs);
    bsal_vector_resize(&self->arcs, size + 1);

    /*
     * A pointer to a new arc.
     */
    arc = bsal_vector_at(&self->arcs, size);

    bsal_assembly_arc_init(arc, type, source,
                                destination,
                                kmer_length, pool, codec);

    /*
     * Verify if it is accepted.
     */
    if (self->enable_redundancy_check) {

        key_length = bsal_assembly_arc_pack_size(arc, kmer_length, codec);
        buffer = bsal_memory_pool_allocate(pool, key_length);

        /*
         * Generate a key to verify if this arc is already in the block.
         * Duplicates are not required anyway.
         */
        bsal_assembly_arc_pack(arc, buffer, kmer_length, codec);

        found = bsal_set_find(&self->set, buffer);

        /*
         * Don't append it if the arc is there already
         */
        if (found) {

            bsal_assembly_arc_destroy(arc, pool);
            bsal_vector_resize(&self->arcs, size);
        } else {

            /*
             * Actually add the key into the
             * set.
             */
            bsal_set_add(&self->set, buffer);
        }

        bsal_memory_pool_free(pool, buffer);
    }
}

int bsal_assembly_arc_block_pack_size(struct bsal_assembly_arc_block *self, int kmer_length, struct bsal_dna_codec *codec)
{
    return bsal_assembly_arc_block_pack_unpack(self, BSAL_PACKER_OPERATION_PACK_SIZE,
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
    struct bsal_assembly_arc *arc;

    bytes = 0;
    bsal_packer_init(&packer, operation, buffer);

    size = bsal_vector_size(&self->arcs);
    bsal_packer_process(&packer, &size, sizeof(size));

    if (operation == BSAL_PACKER_OPERATION_UNPACK) {

        bsal_vector_resize(&self->arcs, size);
    }

    bytes = bsal_packer_get_byte_count(&packer);

    bsal_packer_destroy(&packer);

    for (i = 0; i < size; i++) {

        arc = bsal_vector_at(&self->arcs, i);

        if (operation == BSAL_PACKER_OPERATION_UNPACK) {

            bsal_assembly_arc_init_empty(arc);
        }

        bytes += bsal_assembly_arc_pack_unpack(arc, operation,
                    (char *)buffer + bytes,
                    kmer_length, pool, codec);
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

void bsal_assembly_arc_block_enable_redundancy_check(struct bsal_assembly_arc_block *self)
{
    self->enable_redundancy_check = 1;
}

void bsal_assembly_arc_block_add_arc_copy(struct bsal_assembly_arc_block *self,
                struct bsal_assembly_arc *arc, int kmer_length, struct bsal_dna_codec *codec,
                struct bsal_memory_pool *pool)
{
    struct bsal_assembly_arc new_arc;

    bsal_assembly_arc_init_copy(&new_arc, arc, kmer_length, pool, codec);

    bsal_vector_push_back(&self->arcs, &new_arc);
}
