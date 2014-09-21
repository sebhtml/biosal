
#include "test.h"

#include <genomics/assembly/assembly_arc.h>

#include <genomics/data/dna_kmer.h>
#include <genomics/data/dna_codec.h>

#include <core/system/memory_pool.h>

#include <string.h>

int main(int argc, char **argv)
{
    struct bsal_dna_kmer kmer;
    struct bsal_assembly_arc arc;
    struct bsal_assembly_arc arc2;
    void *buffer;
    int count;
    struct bsal_dna_codec codec;
    struct bsal_memory_pool pool;
    int kmer_length;
    char sequence[] = "ATGATCTGCAGTACTGAC";
    int type;
    int nucleotide;


    BEGIN_TESTS();

    type = BSAL_ARC_TYPE_PARENT;
    nucleotide = BSAL_NUCLEOTIDE_CODE_G;

    kmer_length = strlen(sequence);
    bsal_dna_codec_init(&codec);
    bsal_memory_pool_init(&pool, 1000000, BSAL_MEMORY_POOL_NAME_OTHER);

    bsal_dna_kmer_init(&kmer, sequence, &codec, &pool);

    bsal_assembly_arc_init(&arc, type, &kmer, nucleotide, kmer_length,
                    &pool, &codec);

    count = bsal_assembly_arc_pack_size(&arc, kmer_length, &codec);

    TEST_INT_IS_GREATER_THAN(count, 0);

    buffer = bsal_memory_allocate(count);

    bsal_assembly_arc_pack(&arc, buffer, kmer_length, &codec);

    bsal_assembly_arc_init_empty(&arc2);

    /* unpack and test
     */

    bsal_assembly_arc_unpack(&arc2, buffer, kmer_length, &pool, &codec);

    TEST_BOOLEAN_EQUALS(bsal_assembly_arc_equals(&arc, &arc, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(bsal_assembly_arc_equals(&arc2, &arc2, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(bsal_assembly_arc_equals(&arc, &arc2, kmer_length, &codec), 1);
    TEST_BOOLEAN_EQUALS(bsal_assembly_arc_equals(&arc2, &arc, kmer_length, &codec), 1);

    bsal_assembly_arc_destroy(&arc2, &pool);

    bsal_assembly_arc_destroy(&arc, &pool);

    bsal_dna_kmer_destroy(&kmer, &pool);

    bsal_memory_pool_destroy(&pool);
    bsal_dna_codec_destroy(&codec);

    bsal_memory_free(buffer);

    END_TESTS();

    return 0;
}


