
#include "assembly_arc_block.h"

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
