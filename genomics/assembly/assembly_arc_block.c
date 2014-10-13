
#include "assembly_arc_block.h"

#include <core/system/packer.h>
#include <core/system/debugger.h>

void biosal_assembly_arc_block_free_arcs(struct biosal_assembly_arc_block *self);

void biosal_assembly_arc_block_init(struct biosal_assembly_arc_block *self, struct core_memory_pool *pool,
                int kmer_length, struct biosal_dna_codec *codec)
{
    int key_length;
    struct biosal_assembly_arc arc;

    self->memory_pool = pool;

    CORE_DEBUGGER_LEAK_DETECTION_BEGIN(pool, arc_block_init);

    biosal_assembly_arc_init_mock(&arc, kmer_length, pool, codec);

    key_length = biosal_assembly_arc_pack_size(&arc, kmer_length, codec);

    biosal_assembly_arc_destroy(&arc, pool);

    /*
     * Set the memory pool too.
     * The main use case is to generate stuff to transport between actors so
     * ephemeral memory pool is going to be used.
     */

    core_set_init(&self->set, key_length);
    core_set_set_memory_pool(&self->set, pool);

    core_vector_init(&self->arcs, sizeof(struct biosal_assembly_arc));
    core_vector_set_memory_pool(&self->arcs, pool);

    self->enable_redundancy_check = 0;

    CORE_DEBUGGER_LEAK_DETECTION_END(pool, arc_block_init);
}

void biosal_assembly_arc_block_destroy(struct biosal_assembly_arc_block *self)
{
    core_set_destroy(&self->set);

    /*
     * Arcs are actual objects so we need to free their memory too.
     */
    biosal_assembly_arc_block_free_arcs(self);

    core_vector_destroy(&self->arcs);
}

void biosal_assembly_arc_block_add_arc(struct biosal_assembly_arc_block *self,
                int type, struct biosal_dna_kmer *source,
                int destination,
                int kmer_length, struct biosal_dna_codec *codec)
{
    int key_length;
    void *buffer;
    int found;
    struct biosal_assembly_arc *arc;
    int size;

    size = core_vector_size(&self->arcs);
    core_vector_resize(&self->arcs, size + 1);

    /*
     * A pointer to a new arc.
     */
    arc = core_vector_at(&self->arcs, size);

    biosal_assembly_arc_init(arc, type, source,
                                destination,
                                kmer_length, self->memory_pool, codec);

    /*
     * Verify if it is accepted.
     */
    if (self->enable_redundancy_check) {

        key_length = biosal_assembly_arc_pack_size(arc, kmer_length, codec);
        buffer = core_memory_pool_allocate(self->memory_pool, key_length);

        /*
         * Generate a key to verify if this arc is already in the block.
         * Duplicates are not required anyway.
         */
        biosal_assembly_arc_pack(arc, buffer, kmer_length, codec);

        found = core_set_find(&self->set, buffer);

        /*
         * Don't append it if the arc is there already
         */
        if (found) {

            biosal_assembly_arc_destroy(arc, self->memory_pool);
            core_vector_resize(&self->arcs, size);
        } else {

            /*
             * Actually add the key into the
             * set.
             */
            core_set_add(&self->set, buffer);
        }

        core_memory_pool_free(self->memory_pool, buffer);
    }
}

int biosal_assembly_arc_block_pack_size(struct biosal_assembly_arc_block *self, int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_assembly_arc_block_pack_unpack(self, CORE_PACKER_OPERATION_PACK_SIZE,
                    NULL, kmer_length, codec, NULL);
}

int biosal_assembly_arc_block_pack(struct biosal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct biosal_dna_codec *codec)
{
    return biosal_assembly_arc_block_pack_unpack(self, CORE_PACKER_OPERATION_PACK,
                    buffer, kmer_length, codec, NULL);
}

int biosal_assembly_arc_block_unpack(struct biosal_assembly_arc_block *self, void *buffer,
                int kmer_length, struct biosal_dna_codec *codec,
                struct core_memory_pool *pool)
{
    return biosal_assembly_arc_block_pack_unpack(self, CORE_PACKER_OPERATION_UNPACK,
                    buffer, kmer_length, codec, pool);
}

int biosal_assembly_arc_block_pack_unpack(struct biosal_assembly_arc_block *self, int operation,
                void *buffer, int kmer_length, struct biosal_dna_codec *codec,
                struct core_memory_pool *pool)
{
    int size;
    struct core_packer packer;
    int bytes;
    int i;
    struct biosal_assembly_arc *arc;

    bytes = 0;
    core_packer_init(&packer, operation, buffer);

    size = core_vector_size(&self->arcs);
    core_packer_process(&packer, &size, sizeof(size));

    if (operation == CORE_PACKER_OPERATION_UNPACK) {

        core_vector_resize(&self->arcs, size);
    }

    bytes = core_packer_get_byte_count(&packer);

    core_packer_destroy(&packer);

    for (i = 0; i < size; i++) {

        arc = core_vector_at(&self->arcs, i);

        if (operation == CORE_PACKER_OPERATION_UNPACK) {

            biosal_assembly_arc_init_empty(arc);
        }

        bytes += biosal_assembly_arc_pack_unpack(arc, operation,
                    (char *)buffer + bytes,
                    kmer_length, pool, codec);
    }

    return bytes;
}

struct core_vector *biosal_assembly_arc_block_get_arcs(struct biosal_assembly_arc_block *self)
{
    return &self->arcs;
}

void biosal_assembly_arc_block_reserve(struct biosal_assembly_arc_block *self, int size)
{
    core_vector_reserve(&self->arcs, size);
}

void biosal_assembly_arc_block_enable_redundancy_check(struct biosal_assembly_arc_block *self)
{
    self->enable_redundancy_check = 1;
}

void biosal_assembly_arc_block_add_arc_copy(struct biosal_assembly_arc_block *self,
                struct biosal_assembly_arc *arc, int kmer_length, struct biosal_dna_codec *codec)
{
    struct biosal_assembly_arc new_arc;

    biosal_assembly_arc_init_copy(&new_arc, arc, kmer_length, self->memory_pool, codec);

    core_vector_push_back(&self->arcs, &new_arc);
}

void biosal_assembly_arc_block_clear(struct biosal_assembly_arc_block *self)
{
    biosal_assembly_arc_block_free_arcs(self);

    core_vector_clear(&self->arcs);
    core_set_clear(&self->set);
}

void biosal_assembly_arc_block_free_arcs(struct biosal_assembly_arc_block *self)
{
    int i;
    int size;
    struct biosal_assembly_arc *arc;

    size = core_vector_size(&self->arcs);

    /*
     * Destroy arcs.
     */
    for (i = 0; i < size; i++) {

        arc = core_vector_at(&self->arcs, i);

        biosal_assembly_arc_destroy(arc, self->memory_pool);

        CORE_DEBUGGER_LEAK_CHECK_DOUBLE_FREE(self->memory_pool);
    }
}
