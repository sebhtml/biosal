
#include "vertex_neighborhood.h"

#include "genomics/assembly/assembly_arc.h"
#include "genomics/assembly/assembly_graph_store.h"

#include <stdbool.h>
#include <stdio.h>

#define STEP_GET_MAIN_VERTEX 0
#define STEP_GET_PARENTS 1
#define STEP_GET_CHILDREN 2
#define STEP_FINISH 3

void biosal_vertex_neighborhood_init(struct biosal_vertex_neighborhood *self,
               struct biosal_dna_kmer *kmer,
                int arcs, struct core_vector *graph_stores, int kmer_length,
                struct core_memory_pool *memory, struct biosal_dna_codec *codec,
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

    self->flags = 0;

    if (arcs == BIOSAL_ARC_TYPE_ANY) {
        self->fetch_parents = true;
        self->fetch_children = true;
    } else if (arcs == BIOSAL_ARC_TYPE_PARENT) {
        self->fetch_parents = true;
    } else if (arcs == BIOSAL_ARC_TYPE_CHILD) {
        self->fetch_children = true;
    }

    biosal_dna_kmer_init_copy(&self->main_kmer, kmer, self->kmer_length, self->memory, self->codec);
    biosal_assembly_vertex_init(&self->main_vertex);

    core_vector_init(&self->parent_vertices, sizeof(struct biosal_assembly_vertex));
    core_vector_set_memory_pool(&self->parent_vertices, self->memory);
    core_vector_init(&self->child_vertices, sizeof(struct biosal_assembly_vertex));
    core_vector_set_memory_pool(&self->child_vertices, self->memory);

#if 0
    printf("DEBUG init vertex_neighborhood\n");
#endif
}

void biosal_vertex_neighborhood_destroy(struct biosal_vertex_neighborhood *self)
{
    self->kmer_length = -1;

    self->fetch_parents = false;
    self->fetch_children = false;

    biosal_assembly_vertex_destroy(&self->main_vertex);

    core_vector_destroy(&self->parent_vertices);
    core_vector_destroy(&self->child_vertices);
    biosal_dna_kmer_destroy(&self->main_kmer, self->memory);

    self->memory = NULL;
    self->codec = NULL;
    self->actor = NULL;
}

int biosal_vertex_neighborhood_receive(struct biosal_vertex_neighborhood *self, struct thorium_message *message)
{
    int tag;
    struct biosal_assembly_vertex vertex;
    void *buffer;

    if (message == NULL) {
        return biosal_vertex_neighborhood_execute(self);
    }

    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_ASSEMBLY_GET_VERTEX_REPLY) {

        biosal_assembly_vertex_init_empty(&vertex);
        biosal_assembly_vertex_unpack(&vertex, buffer);

        if (self->step == STEP_GET_MAIN_VERTEX) {

            biosal_assembly_vertex_init_copy(&self->main_vertex, &vertex);

#if 0
            printf("Main vertex: \n");
            biosal_assembly_vertex_print(&self->main_vertex);
            printf("\n");
#endif

            self->step = STEP_GET_PARENTS;

            return biosal_vertex_neighborhood_execute(self);

        } else if (self->step == STEP_GET_PARENTS) {

            core_vector_push_back(&self->parent_vertices, &vertex);

            return biosal_vertex_neighborhood_execute(self);

        } else if (self->step == STEP_GET_CHILDREN) {
            core_vector_push_back(&self->child_vertices, &vertex);

            return biosal_vertex_neighborhood_execute(self);
        }
    }

    return 0;
}

void biosal_vertex_neighborhood_init_empty(struct biosal_vertex_neighborhood *self)
{
    self->kmer_length = -1;
    self->memory = NULL;
    self->codec = NULL;

    self->fetch_parents = false;
    self->fetch_children = false;

    biosal_dna_kmer_init_empty(&self->main_kmer);
    biosal_assembly_vertex_init_empty(&self->main_vertex);

    core_vector_init(&self->parent_vertices, 0);
    core_vector_init(&self->child_vertices, 0);
}

void biosal_vertex_neighborhood_get_remote_memory(struct biosal_vertex_neighborhood *self, struct biosal_dna_kmer *kmer)
{
    struct core_memory_pool *ephemeral_memory;
    struct thorium_message new_message;
    void *new_buffer;
    int new_count;
    int store_index;
    int store;
    int size;

    ephemeral_memory = thorium_actor_get_ephemeral_memory(self->actor);

    new_count = biosal_dna_kmer_pack_size(kmer, self->kmer_length,
                self->codec);
    new_buffer = thorium_actor_allocate(self->actor, new_count);
    biosal_dna_kmer_pack(kmer, new_buffer, self->kmer_length, self->codec);

    size = core_vector_size(self->graph_stores);
    store_index = biosal_dna_kmer_store_index(kmer, size, self->kmer_length,
                self->codec, ephemeral_memory);
    store = core_vector_at_as_int(self->graph_stores, store_index);

    thorium_message_init(&new_message, ACTION_ASSEMBLY_GET_VERTEX, new_count, new_buffer);
    thorium_actor_send(self->actor, store, &new_message);
    thorium_message_destroy(&new_message);
}

int biosal_vertex_neighborhood_execute(struct biosal_vertex_neighborhood *self)
{
    int actual;
    int expected;
    int edge_index;
    int code;
    struct biosal_dna_kmer other_kmer;

    if (self->step == STEP_GET_MAIN_VERTEX) {

        biosal_vertex_neighborhood_get_remote_memory(self, &self->main_kmer);

#if 0
        printf("neighborhood STEP_GET_MAIN_VERTEX\n");
#endif

    } else if (self->step == STEP_GET_PARENTS) {

#if 0
        printf("neighborhood STEP_GET_PARENTS\n");
#endif

        actual = core_vector_size(&self->parent_vertices);
        expected = biosal_assembly_vertex_parent_count(&self->main_vertex);

        /* DONE: Fix this. */

        if (!self->fetch_parents)
            expected = 0;

        if (actual == expected) {

#if 0
            printf("Fetched %d parents.\n", expected);
#endif

            self->step = STEP_GET_CHILDREN;

            return biosal_vertex_neighborhood_execute(self);
        } else {

            /*
             * Fetch a parent.
             */
            edge_index = actual;
            code = biosal_assembly_vertex_get_parent(&self->main_vertex,
                            edge_index);

            biosal_dna_kmer_init_as_parent(&other_kmer, &self->main_kmer,
                            code, self->kmer_length, self->memory,
                            self->codec);

            /*
             * Do some remote-memory access.
             */
            biosal_vertex_neighborhood_get_remote_memory(self, &other_kmer);
            biosal_dna_kmer_destroy(&other_kmer, self->memory);
        }
    } else if (self->step == STEP_GET_CHILDREN) {
#if 0
        printf("neighborhood STEP_GET_CHILDREN\n");
#endif

        actual = core_vector_size(&self->child_vertices);
        expected = biosal_assembly_vertex_child_count(&self->main_vertex);

        if (!self->fetch_children)
            expected = 0;

        if (actual == expected) {
            self->step = STEP_FINISH;

            return biosal_vertex_neighborhood_execute(self);
        } else {

            /*
             * Fetch a child
             */
            edge_index = actual;
            code = biosal_assembly_vertex_get_child(&self->main_vertex,
                            edge_index);

            biosal_dna_kmer_init_as_child(&other_kmer, &self->main_kmer,
                            code, self->kmer_length, self->memory,
                            self->codec);

            /*
             * Do some remote-memory access.
             */
            biosal_vertex_neighborhood_get_remote_memory(self, &other_kmer);
            biosal_dna_kmer_destroy(&other_kmer, self->memory);
        }
    } else if (self->step == STEP_FINISH) {

#if 0
        printf("neighborhood, finished, coverage %d parents %d children %d"
                        " fetched parents %d fetched children %d\n",
                        biosal_assembly_vertex_coverage_depth(&self->main_vertex),
                        biosal_assembly_vertex_parent_count(&self->main_vertex),
                        biosal_assembly_vertex_child_count(&self->main_vertex),
                        (int)core_vector_size(&self->parent_vertices),
                        (int)core_vector_size(&self->child_vertices));
#endif

        return 1;
    }

    if (self->step == STEP_FINISH) {
        return 1;
    } else {
        return 0;
    }
}

struct biosal_assembly_vertex *biosal_vertex_neighborhood_vertex(struct biosal_vertex_neighborhood *self)
{
    return &self->main_vertex;
}

struct biosal_assembly_vertex *biosal_vertex_neighborhood_parent(struct biosal_vertex_neighborhood *self, int i)
{
    return core_vector_at(&self->parent_vertices, i);
}

struct biosal_assembly_vertex *biosal_vertex_neighborhood_child(struct biosal_vertex_neighborhood *self, int i)
{
    return core_vector_at(&self->child_vertices, i);
}
