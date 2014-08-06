
#include "assembly_connectivity.h"

#include "assembly_arc.h"

#include <core/helpers/bitmap.h>

#include <genomics/data/dna_codec.h>

void bsal_assembly_connectivity_init(struct bsal_assembly_connectivity *self)
{
    bsal_assembly_connectivity_delete_parent(self, BSAL_NUCLEOTIDE_CODE_A);
    bsal_assembly_connectivity_delete_parent(self, BSAL_NUCLEOTIDE_CODE_T);
    bsal_assembly_connectivity_delete_parent(self, BSAL_NUCLEOTIDE_CODE_C);
    bsal_assembly_connectivity_delete_parent(self, BSAL_NUCLEOTIDE_CODE_G);

    bsal_assembly_connectivity_delete_child(self, BSAL_NUCLEOTIDE_CODE_A);
    bsal_assembly_connectivity_delete_child(self, BSAL_NUCLEOTIDE_CODE_T);
    bsal_assembly_connectivity_delete_child(self, BSAL_NUCLEOTIDE_CODE_C);
    bsal_assembly_connectivity_delete_child(self, BSAL_NUCLEOTIDE_CODE_G);
}

void bsal_assembly_connectivity_destroy(struct bsal_assembly_connectivity *self)
{
    bsal_assembly_connectivity_delete_parent(self, BSAL_NUCLEOTIDE_CODE_A);
    bsal_assembly_connectivity_delete_parent(self, BSAL_NUCLEOTIDE_CODE_T);
    bsal_assembly_connectivity_delete_parent(self, BSAL_NUCLEOTIDE_CODE_C);
    bsal_assembly_connectivity_delete_parent(self, BSAL_NUCLEOTIDE_CODE_G);

    bsal_assembly_connectivity_delete_child(self, BSAL_NUCLEOTIDE_CODE_A);
    bsal_assembly_connectivity_delete_child(self, BSAL_NUCLEOTIDE_CODE_T);
    bsal_assembly_connectivity_delete_child(self, BSAL_NUCLEOTIDE_CODE_C);
    bsal_assembly_connectivity_delete_child(self, BSAL_NUCLEOTIDE_CODE_G);
}

int bsal_assembly_connectivity_child_count(struct bsal_assembly_connectivity *self)
{
    return bsal_assembly_connectivity_get_count(self, BSAL_ARC_TYPE_CHILD);
}

int bsal_assembly_connectivity_get_count(struct bsal_assembly_connectivity *self, int type)
{
    int count;
    int i;
    int value;
    int offset;

    count = 0;

    for (i = 0; i < 4; i++) {

        offset = 0;

        if (type == BSAL_ARC_TYPE_PARENT) {
            offset = bsal_assembly_connectivity_parent_offset(i);

        } else if (type == BSAL_ARC_TYPE_CHILD) {

            offset = bsal_assembly_connectivity_child_offset(i);
        }

        value = bsal_bitmap_get_bit_value_uint8_t(&self->bitmap,
                offset);

        count += value;
    }

    return count;
}

void bsal_assembly_connectivity_add_child(struct bsal_assembly_connectivity *self, int symbol_code)
{
    bsal_bitmap_set_bit_value_uint8_t(&self->bitmap,
                bsal_assembly_connectivity_child_offset(symbol_code),
                    BSAL_BIT_ONE);
}

void bsal_assembly_connectivity_delete_child(struct bsal_assembly_connectivity *self, int symbol_code)
{
    bsal_bitmap_set_bit_value_uint8_t(&self->bitmap,
                bsal_assembly_connectivity_child_offset(symbol_code),
                    BSAL_BIT_ZERO);

}

int bsal_assembly_connectivity_get_element(struct bsal_assembly_connectivity *self, int index, int type)
{
    int bits_to_skip;
    int i;
    int skipped;
    int value;
    int offset;

    bits_to_skip = index;
    skipped = -1;

    for (i = 0; i < 4; i++) {

        offset = 0;

        if (type == BSAL_ARC_TYPE_CHILD) {
            offset = bsal_assembly_connectivity_child_offset(i);

        } else if (type == BSAL_ARC_TYPE_PARENT) {

            offset = bsal_assembly_connectivity_parent_offset(i);
        }

        value = bsal_bitmap_get_bit_value_uint8_t(&self->bitmap, offset);

        if (value) {
            skipped++;
        }

        if (skipped == bits_to_skip) {
            return i;
        }
    }

    return -1;
}

int bsal_assembly_connectivity_get_child(struct bsal_assembly_connectivity *self, int index)
{
    return bsal_assembly_connectivity_get_element(self, index, BSAL_ARC_TYPE_CHILD);
}


int bsal_assembly_connectivity_parent_count(struct bsal_assembly_connectivity *self)
{
    return bsal_assembly_connectivity_get_count(self, BSAL_ARC_TYPE_PARENT);
}

void bsal_assembly_connectivity_add_parent(struct bsal_assembly_connectivity *self, int symbol_code)
{
    bsal_bitmap_set_bit_value_uint8_t(&self->bitmap,
                bsal_assembly_connectivity_parent_offset(symbol_code),
                    BSAL_BIT_ONE);

}

void bsal_assembly_connectivity_delete_parent(struct bsal_assembly_connectivity *self, int symbol_code)
{
    bsal_bitmap_set_bit_value_uint8_t(&self->bitmap,
                bsal_assembly_connectivity_parent_offset(symbol_code),
                    BSAL_BIT_ZERO);

}

int bsal_assembly_connectivity_get_parent(struct bsal_assembly_connectivity *self, int index)
{
    return bsal_assembly_connectivity_get_element(self, index, BSAL_ARC_TYPE_PARENT);
}

int bsal_assembly_connectivity_parent_offset(int code)
{
    return BSAL_ASSEMBLY_CONNECTIVITY_OFFSET_PARENTS + code;
}

int bsal_assembly_connectivity_child_offset(int code)
{
    return BSAL_ASSEMBLY_CONNECTIVITY_OFFSET_CHILDREN + code;
}


