
#include "assembly_arc_block.h"

#include <core/system/packer.h>
#include <core/system/debugger.h>

void biosal_assembly_arc_block_init(struct biosal_assembly_arc_block *self, struct biosal_memory_pool *pool,
                int kmer_length, struct biosal_dna_codec *codec)
{
    int key_length;
    struct biosal_assembly_arc arc;

    BIOSAL_DEBUGGER_LEAK_DETECTION_BEGIN(pool, arc_block_init);

    biosal_assembly_arc_init_mock(&arc, kmer_length, pool, codec);

    key_length = biosal_assembly_arc_pack_size(&arc, kmer_length, codec);

    biosal_assembly_arc_destroy(&arc, pool);

    /*
     * Set the memory pool too.
     * The main use case is to generate stuff to transport between actors so
     * ephemeral memory pool is going to be used.
     */

    biosal_set_init(&self->set, key_length);
    biosal_set_set_memory_pool(&self->set, pool);

    biosal_vector_init(&self->arcs, sizeof(struct biosal_assembly_arc));
    biosal_vector_set_memory_pool(&self->arcs, pool);

    self->enable_redundancy_check = 0;

    BIOSAL_DEBUGGER_LEAK_DETECTION_END(pool, arc_block_init);
}

void biosal_assembly_arc_block_destroy(struct biosal_assembly_arc_block *self, struct biosal_memory_pool *pool)
{
    int i;
    int size;
    struct biosal_assembly_arc *arc;

    biosal_set_destroy(&self->set);

    size = biosal_vector_size(&self->arcs);

    /*
     * Destroy arcs.
     */
    for (i = 0; i < size; i++) {

        arc = biosal_vector_at(&self->arcs, i);

        biosal_assembly_arc_destroy(arc, pool);

        BIOSAL_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(pool);
    }

    biosal_vector_destroy(&self->arcs);
}

void biosal_assembly_arc_block_add_arc(struct biosal_assembly_arc_block *self,
                int type, struct biosal_dna_kmer *source,
                int destination,
                int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *pool)
{
    int key_length;
    void *buffer;
    int found;
    struct biosal_assembly_arc *arc;
    int size;

    size = biosal_vector_size(&self->arcs);
    biosal_vector_resize(&self->arcs, size + 1);

    /*
     * A pointer to a new arc.
     */
    arc = biosal_vector_at(&self->arcs, size);

    biosal_assembly_arc_init(arc, type, source,
                                destination,
                                kmer_length, pool, codec);

    /*
     * Verify if it is accepted.
     */
    if (self->enable_redundancy_check) {

        key_length = biosal_assembly_arc_pack_size(arc, kmer_length, codec);
        buffer = biosal_memory_pool_allocate(pool, key_length);

        /*
         * Generate a key to verify if this arc is already in the block.
         * Duplicates are not required anyway.
         */
        biosal_assembly_arc_pack(arc, buffer, kmer_length, codec);

        found = biosal_set_find(&self->set, buffer);

        /*
         * Don't append it if the arc is there already
         */
        if (found) {

            biosal_assembly_arc_destroy(arc, pool);
            biosal_vector_resize(&self->arcs, size);
        } else {

            /*
             * Actually add the key into the
             * set.
             */
            biosal_set_add(&self->set, buffer);
        }

        biosal_memory_pool_free(pool, buffer);
    }
}

int biosal_assembly_arc_block_pack_size(struct biosal_assembly_arc_block *self, int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_assembly_arc_block_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK_SIZE,
                    NULL, kmer_length, codec, NULL);
}

int biosal_assembly_arc_block_pack(struct biosal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_assembly_arc_block_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK,
                    buffer, kmer_length, codec, NULL);
}

int biosal_assembly_arc_block_unpack(struct biosal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *pool)
{
    return biosal_assembly_arc_block_pack_unpack(self, BIOSAL_PACKER_OPERATION_UNPACK,
                    buffer, kmer_length, codec, pool);
}

int biosal_assembly_arc_block_pack_unpack(struct biosal_assembly_arc_block *self, int operation,
                void *buffer, int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *pool)
{
    int size;
    struct biosal_packer packer;
    int bytes;
    int i;
    struct biosal_assembly_arc *arc;

    bytes = 0;
    biosal_packer_init(&packer, operation, buffer);

    size = biosal_vector_size(&self->arcs);
    biosal_packer_process(&packer, &size, sizeof(size));

    if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {

        biosal_vector_resize(&self->arcs, size);
    }

    bytes = biosal_packer_get_byte_count(&packer);

    biosal_packer_destroy(&packer);

    for (i = 0; i < size; i++) {

        arc = biosal_vector_at(&self->arcs, i);

        if (operation == BIOSAL_PACKER_OPERATION_UNPACK) {

            biosal_assembly_arc_init_empty(arc);
        }

        bytes += biosal_assembly_arc_pack_unpack(arc, operation,
                    (char *)buffer + bytes,
                    kmer_length, pool, codec);
    }

    return bytes;
}

struct biosal_vector *biosal_assembly_arc_block_get_arcs(struct biosal_assembly_arc_block *self)
{
    return &self->arcs;
}

void biosal_assembly_arc_block_reserve(struct biosal_assembly_arc_block *self, int size)
{
    biosal_vector_reserve(&self->arcs, size);
}

void biosal_assembly_arc_block_enable_redundancy_check(struct biosal_assembly_arc_block *self)
{
    self->enable_redundancy_check = 1;
}

void biosal_assembly_arc_block_add_arc_copy(struct biosal_assembly_arc_block *self,
                struct biosal_assembly_arc *arc, int kmer_length, struct biosal_dna_codec *codec,
                struct biosal_memory_pool *pool)
{
    struct biosal_assembly_arc new_arc;

    biosal_assembly_arc_init_copy(&new_arc, arc, kmer_length, pool, codec);

    biosal_vector_push_back(&self->arcs, &new_arc);
}
