
#include "vertex_neighborhood.h"

#include "genomics/assembly/assembly_arc.h"
#include "genomics/assembly/assembly_graph_store.h"

#include <stdbool.h>
#include <stdio.h>

#define STEP_GET_MAIN_VERTEX 0
#define STEP_GET_PARENTS 1
#define STEP_GET_CHILDREN 2
#define STEP_FINISH 3

void bsal_vertex_neighborhood_init(struct bsal_vertex_neighborhood *self,
               struct bsal_dna_kmer *kmer,
                int arcs, struct bsal_vector *graph_stores, int kmer_length,
                struct bsal_memory_pool *memory, struct bsal_dna_codec *codec,
                struct thorium_actor *actor)
{
    self->kmer_length = kmer_length;
    self->memory = memory;
    self->codec = codec;
    self->actor = actor;
    self->graph_stores = graph_stores;

    self->fetch_parents = false;
    self->fetch_children = false;

    self->step = STEP_GET_MAIN_VERTEX;

    if (arcs == BSAL_ARC_TYPE_ANY) {
        self->fetch_parents = true;
        self->fetch_children = true;
    } else if (arcs == BSAL_ARC_TYPE_PARENT) {
        self->fetch_parents = true;
    } else if (arcs == BSAL_ARC_TYPE_CHILD) {
        self->fetch_children = true;
    }

    bsal_dna_kmer_init_copy(&self->main_kmer, kmer, self->kmer_length, self->memory, self->codec);
    bsal_assembly_vertex_init(&self->main_vertex);

    bsal_vector_init(&self->parent_vertices, sizeof(struct bsal_assembly_vertex));
    bsal_vector_set_memory_pool(&self->parent_vertices, self->memory);
    bsal_vector_init(&self->child_vertices, sizeof(struct bsal_assembly_vertex));
    bsal_vector_set_memory_pool(&self->child_vertices, self->memory);

#if 0
    printf("DEBUG init vertex_neighborhood\n");
#endif
}

void bsal_vertex_neighborhood_destroy(struct bsal_vertex_neighborhood *self)
{
    self->kmer_length = -1;

    self->fetch_parents = false;
    self->fetch_children = false;

    bsal_assembly_vertex_destroy(&self->main_vertex);

    bsal_vector_destroy(&self->parent_vertices);
    bsal_vector_destroy(&self->child_vertices);
    bsal_dna_kmer_destroy(&self->main_kmer, self->memory);

    self->memory = NULL;
    self->codec = NULL;
    self->actor = NULL;
}

int bsal_vertex_neighborhood_fetch(struct bsal_vertex_neighborhood *self, struct thorium_message *message)
{
    int tag;
    struct bsal_assembly_vertex vertex;
    void *buffer;

    if (message == NULL) {
        return bsal_vertex_neighborhood_do_something(self);
    }

    tag = thorium_message_tag(message);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_ASSEMBLY_GET_VERTEX_REPLY) {

        bsal_assembly_vertex_init_empty(&vertex);
        bsal_assembly_vertex_unpack(&vertex, buffer);

        if (self->step == STEP_GET_MAIN_VERTEX) {

            bsal_assembly_vertex_init_copy(&self->main_vertex, &vertex);

#if 0
            printf("Main vertex: \n");
            bsal_assembly_vertex_print(&self->main_vertex);
            printf("\n");
#endif

            self->step = STEP_GET_PARENTS;

            return bsal_vertex_neighborhood_do_something(self);

        } else if (self->step == STEP_GET_PARENTS) {

            bsal_vector_push_back(&self->parent_vertices, &vertex);

            return bsal_vertex_neighborhood_do_something(self);

        } else if (self->step == STEP_GET_CHILDREN) {
            bsal_vector_push_back(&self->child_vertices, &vertex);

            return bsal_vertex_neighborhood_do_something(self);
        }
    }

    return 0;
}

void bsal_vertex_neighborhood_init_empty(struct bsal_vertex_neighborhood *self)
{
    self->kmer_length = -1;
    self->memory = NULL;
    self->codec = NULL;

    self->fetch_parents = false;
    self->fetch_children = false;

    bsal_dna_kmer_init_empty(&self->main_kmer);
    bsal_assembly_vertex_init_empty(&self->main_vertex);
}

void bsal_vertex_neighborhood_fetch_remote_memory(struct bsal_vertex_neighborhood *self, struct bsal_dna_kmer *kmer)
{
    struct bsal_memory_pool *ephemeral_memory;
    struct thorium_message new_message;
    void *new_buffer;
    int new_count;
    int store_index;
    int store;
    int size;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self->actor);

    new_count = bsal_dna_kmer_pack_size(kmer, self->kmer_length,
                self->codec);
    new_buffer = bsal_memory_pool_allocate(ephemeral_memory, new_count);
    bsal_dna_kmer_pack(kmer, new_buffer, self->kmer_length, self->codec);

    size = bsal_vector_size(self->graph_stores);
    store_index = bsal_dna_kmer_store_index(kmer, size, self->kmer_length,
                self->codec, ephemeral_memory);
    store = bsal_vector_at_as_int(self->graph_stores, store_index);

    thorium_message_init(&new_message, ACTION_ASSEMBLY_GET_VERTEX, new_count, new_buffer);
    thorium_actor_send(self->actor, store, &new_message);
    thorium_message_destroy(&new_message);

    bsal_memory_pool_free(ephemeral_memory, new_buffer);
}

int bsal_vertex_neighborhood_do_something(struct bsal_vertex_neighborhood *self)
{
    int actual;
    int expected;

    if (self->step == STEP_GET_MAIN_VERTEX) {

        bsal_vertex_neighborhood_fetch_remote_memory(self, &self->main_kmer);

#if 0
        printf("neighborhood STEP_GET_MAIN_VERTEX\n");
#endif

    } else if (self->step == STEP_GET_PARENTS) {

#if 0
        printf("neighborhood STEP_GET_PARENTS\n");
#endif

        actual = bsal_vector_size(&self->parent_vertices);
        expected = bsal_assembly_vertex_parent_count(&self->main_vertex);

        /* TODO Fix this. */
        expected = 0;

        if (actual == expected) {
            self->step = STEP_GET_CHILDREN;

            return bsal_vertex_neighborhood_do_something(self);
        } else {

            /*
             * Fetch a parent.
             */
        }
    } else if (self->step == STEP_GET_CHILDREN) {
#if 0
        printf("neighborhood STEP_GET_CHILDREN\n");
#endif

        actual = bsal_vector_size(&self->child_vertices);
        expected = bsal_assembly_vertex_child_count(&self->main_vertex);

        expected = 0;

        if (actual == expected) {
            self->step = STEP_FINISH;

            return bsal_vertex_neighborhood_do_something(self);
        }
    } else if (self->step == STEP_FINISH) {

        return 1;
    }

    if (self->step == STEP_FINISH) {
        return 1;
    } else {
        return 0;
    }
}
