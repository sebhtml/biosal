
#ifndef BIOSAL_UNITIG_WALKER_H
#define BIOSAL_UNITIG_WALKER_H

#include "unitig_heuristic.h"

#include <engine/thorium/actor.h>

#include <core/file_storage/output/buffered_file_writer.h>

#include <genomics/data/dna_codec.h>
#include <genomics/data/dna_kmer.h>

#include <genomics/assembly/assembly_vertex.h>

#include <core/structures/string.h>

#include <core/system/memory_pool.h>

#define SCRIPT_UNITIG_WALKER 0x78390b2f

#define ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT 0x0000122a
#define ACTION_ASSEMBLY_GET_VERTICES_AND_SELECT_REPLY 0x00000a0b

/*
#define BIOSAL_UNITIG_WALKER_USE_PRIVATE_FILE
*/

/*
 * A dummy walker to test the concept.
 */
struct biosal_unitig_walker {
    struct core_vector graph_stores;
    int kmer_length;
    struct biosal_dna_codec codec;
    struct core_memory_pool memory_pool;
    int skipped_at_start_used;
    int skipped_at_start_not_unitig;
    struct core_map path_statuses;
    int source;
    int current_is_circular;

    /*
     * The next store index to use.
     */
    int store_index;

    int dried_stores;

#ifdef BIOSAL_UNITIG_WALKER_USE_PRIVATE_FILE
    struct biosal_buffered_file_writer writer;
    struct core_string file_path;
#endif

    int has_starting_vertex;

    struct biosal_dna_kmer starting_kmer;
    struct biosal_assembly_vertex starting_vertex;
    struct core_vector left_path;
    struct core_vector right_path;

    struct biosal_dna_kmer current_kmer;
    struct biosal_assembly_vertex current_vertex;

    int current_child;
    struct core_vector child_kmers;
    struct core_vector child_vertices;

    int current_parent;
    struct core_vector parent_kmers;
    struct core_vector parent_vertices;

    int path_index;

    int key_length;
    struct core_set visited;
    int fetch_operation;
    int select_operation;

    struct biosal_unitig_heuristic heuristic;

    int writer_process;
    int start_messages;

    int number_of_visited_vertices_in_common;
};

extern struct thorium_script biosal_unitig_walker_script;

#endif
