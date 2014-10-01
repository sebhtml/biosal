
#include <genomics/assembly/assembly_vertex.h>

#include "test.h"

int main(int argc, char **argv)
{
    struct biosal_assembly_vertex vertex;
    int i;
    int has_c;
    int has_g;
    int has_t;
    int count;
    int code;

    BEGIN_TESTS();

    biosal_assembly_vertex_init(&vertex);

    TEST_INT_EQUALS(biosal_assembly_vertex_parent_count(&vertex), 0);
    TEST_INT_EQUALS(biosal_assembly_vertex_child_count(&vertex), 0);


    /*
     * Add some links
     */

    biosal_assembly_vertex_add_child(&vertex, BIOSAL_NUCLEOTIDE_CODE_G);
    biosal_assembly_vertex_add_child(&vertex, BIOSAL_NUCLEOTIDE_CODE_C);
    biosal_assembly_vertex_add_child(&vertex, BIOSAL_NUCLEOTIDE_CODE_T);

    TEST_INT_EQUALS(biosal_assembly_vertex_parent_count(&vertex), 0);
    TEST_INT_EQUALS(biosal_assembly_vertex_child_count(&vertex), 3);

    has_g = 0;
    has_c = 0;
    has_t = 0;

    count = biosal_assembly_vertex_child_count(&vertex);


    for (i = 0; i < count; i++) {

        code = biosal_assembly_vertex_get_child(&vertex, i);

        if (code == BIOSAL_NUCLEOTIDE_CODE_G) {
            has_g = 1;
        }

        if (code == BIOSAL_NUCLEOTIDE_CODE_C) {
            has_c = 1;
        }

        if (code == BIOSAL_NUCLEOTIDE_CODE_T) {
            has_t = 1;
        }
    }

    TEST_INT_EQUALS(has_t, 1);
    TEST_INT_EQUALS(has_g, 1);
    TEST_INT_EQUALS(has_c, 1);

    biosal_assembly_vertex_delete_child(&vertex, BIOSAL_NUCLEOTIDE_CODE_C);

    has_g = 0;
    has_c = 0;
    has_t = 0;

    count = biosal_assembly_vertex_child_count(&vertex);


    for (i = 0; i < count; i++) {

        code = biosal_assembly_vertex_get_child(&vertex, i);

        if (code == BIOSAL_NUCLEOTIDE_CODE_G) {
            has_g = 1;
        }

        if (code == BIOSAL_NUCLEOTIDE_CODE_C) {
            has_c = 1;
        }

        if (code == BIOSAL_NUCLEOTIDE_CODE_T) {
            has_t = 1;
        }
    }

    TEST_INT_EQUALS(has_t, 1);
    TEST_INT_EQUALS(has_g, 1);
    TEST_INT_EQUALS(has_c, 0);


    code = biosal_assembly_vertex_get_parent(&vertex, 0);

    TEST_INT_NOT_EQUALS(code, BIOSAL_NUCLEOTIDE_CODE_A);
    TEST_INT_NOT_EQUALS(code, BIOSAL_NUCLEOTIDE_CODE_C);
    TEST_INT_NOT_EQUALS(code, BIOSAL_NUCLEOTIDE_CODE_G);
    TEST_INT_NOT_EQUALS(code, BIOSAL_NUCLEOTIDE_CODE_T);

    code = biosal_assembly_vertex_get_child(&vertex, 99);

    TEST_INT_NOT_EQUALS(code, BIOSAL_NUCLEOTIDE_CODE_A);
    TEST_INT_NOT_EQUALS(code, BIOSAL_NUCLEOTIDE_CODE_C);
    TEST_INT_NOT_EQUALS(code, BIOSAL_NUCLEOTIDE_CODE_G);
    TEST_INT_NOT_EQUALS(code, BIOSAL_NUCLEOTIDE_CODE_T);


    biosal_assembly_vertex_add_parent(&vertex, BIOSAL_NUCLEOTIDE_CODE_G);

    TEST_INT_EQUALS(biosal_assembly_vertex_parent_count(&vertex), 1);

    has_g = 0;
    has_c = 0;
    has_t = 0;

    count = biosal_assembly_vertex_parent_count(&vertex);


    for (i = 0; i < count; i++) {

        code = biosal_assembly_vertex_get_parent(&vertex, i);

        if (code == BIOSAL_NUCLEOTIDE_CODE_G) {
            has_g = 1;
        }

        if (code == BIOSAL_NUCLEOTIDE_CODE_C) {
            has_c = 1;
        }

        if (code == BIOSAL_NUCLEOTIDE_CODE_T) {
            has_t = 1;
        }
    }

    TEST_INT_EQUALS(has_t, 0);
    TEST_INT_EQUALS(has_g, 1);
    TEST_INT_EQUALS(has_c, 0);



    biosal_assembly_vertex_destroy(&vertex);

    END_TESTS();

    return 0;
}
