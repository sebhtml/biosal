
#include "test.h"

#include <genomics/assembly/assembly_arc.h>

#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_codec.h>

#include <core/system/memory_pool.h>

#include <string.h>

int main(int argc, char **argv)
{
    struct biosal_dna_kmer kmer;
    struct biosal_assembly_arc arc;
    struct biosal_assembly_arc arc2;
    void *buffer;
    int count;
    struct biosal_dna_codec codec;
    struct core_memory_pool pool;
    int kmer_length;
    char sequence[] = "ATGATCTGCAGTACTGAC";
    int type;
    int nucleotide;


    BEGIN_TESTS();

    type = BIOSAL_ARC_TYPE_PARENT;
    nucleotide = BIOSAL_NUCLEOTIDE_CODE_G;

    kmer_length = strlen(sequence);
    biosal_dna_codec_init(&codec);
    core_memory_pool_init(&pool, 1000000, -1);

    biosal_dna_kmer_init(&kmer, sequence, &codec, &pool);

    biosal_assembly_arc_init(&arc, type, &kmer, nucleotide, kmer_length,
                    &pool, &codec);

    count = biosal_assembly_arc_pack_size(&arc, kmer_length, &codec);

    TEST_INT_IS_GREATER_THAN(count, 0);

    buffer = core_memory_allocate(count, -1);

    biosal_assembly_arc_pack(&arc, buffer, kmer_length, &codec);

    biosal_assembly_arc_init_empty(&arc2);

    /* unpack and test
     */

    biosal_assembly_arc_unpack(&arc2, buffer, kmer_length, &pool, &codec);

    TEST_BOOLEAN_EQUALS(biosal_assembly_arc_equals(&arc, &arc, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(biosal_assembly_arc_equals(&arc2, &arc2, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(biosal_assembly_arc_equals(&arc, &arc2, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(biosal_assembly_arc_equals(&arc2, &arc, kmer_length, &codec), 1);

    biosal_assembly_arc_destroy(&arc2, &pool);

    biosal_assembly_arc_destroy(&arc, &pool);

    biosal_dna_kmer_destroy(&kmer, &pool);

    core_memory_pool_destroy(&pool);
    biosal_dna_codec_destroy(&codec);

    core_memory_free(buffer, -1);

    END_TESTS();

    return 0;
}


