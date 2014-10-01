
#include "assembly_connectivity.h"

#include "assembly_arc.h"

#include <core/helpers/bitmap.h>
#include <core/system/packer.h>

#include <genomics/data/dna_codec.h>

#include <stdio.h>

void biosal_assembly_connectivity_init(struct biosal_assembly_connectivity *self)
{
    biosal_assembly_connectivity_delete_parent(self, BIOSAL_NUCLEOTIDE_CODE_A);
    biosal_assembly_connectivity_delete_parent(self, BIOSAL_NUCLEOTIDE_CODE_T);
    biosal_assembly_connectivity_delete_parent(self, BIOSAL_NUCLEOTIDE_CODE_C);
    biosal_assembly_connectivity_delete_parent(self, BIOSAL_NUCLEOTIDE_CODE_G);

    biosal_assembly_connectivity_delete_child(self, BIOSAL_NUCLEOTIDE_CODE_A);
    biosal_assembly_connectivity_delete_child(self, BIOSAL_NUCLEOTIDE_CODE_T);
    biosal_assembly_connectivity_delete_child(self, BIOSAL_NUCLEOTIDE_CODE_C);
    biosal_assembly_connectivity_delete_child(self, BIOSAL_NUCLEOTIDE_CODE_G);
}

void biosal_assembly_connectivity_destroy(struct biosal_assembly_connectivity *self)
{
    biosal_assembly_connectivity_delete_parent(self, BIOSAL_NUCLEOTIDE_CODE_A);
    biosal_assembly_connectivity_delete_parent(self, BIOSAL_NUCLEOTIDE_CODE_T);
    biosal_assembly_connectivity_delete_parent(self, BIOSAL_NUCLEOTIDE_CODE_C);
    biosal_assembly_connectivity_delete_parent(self, BIOSAL_NUCLEOTIDE_CODE_G);

    biosal_assembly_connectivity_delete_child(self, BIOSAL_NUCLEOTIDE_CODE_A);
    biosal_assembly_connectivity_delete_child(self, BIOSAL_NUCLEOTIDE_CODE_T);
    biosal_assembly_connectivity_delete_child(self, BIOSAL_NUCLEOTIDE_CODE_C);
    biosal_assembly_connectivity_delete_child(self, BIOSAL_NUCLEOTIDE_CODE_G);
}

int biosal_assembly_connectivity_child_count(struct biosal_assembly_connectivity *self)
{
    return biosal_assembly_connectivity_get_count(self, BIOSAL_ARC_TYPE_CHILD);
}

int biosal_assembly_connectivity_get_count(struct biosal_assembly_connectivity *self, int type)
{
    int count;
    int i;
    int value;
    int offset;

    count = 0;

    for (i = 0; i < 4; i++) {

        offset = 0;

        if (type == BIOSAL_ARC_TYPE_PARENT) {
            offset = biosal_assembly_connectivity_parent_offset(i);

        } else if (type == BIOSAL_ARC_TYPE_CHILD) {

            offset = biosal_assembly_connectivity_child_offset(i);
        }

        value = biosal_bitmap_get_bit_uint8_t(&self->bitmap,
                offset);

        count += value;
    }

    return count;
}

void biosal_assembly_connectivity_add_child(struct biosal_assembly_connectivity *self, int symbol_code)
{
    biosal_bitmap_set_bit_value_uint8_t(&self->bitmap,
                biosal_assembly_connectivity_child_offset(symbol_code),
                    BIOSAL_BIT_ONE);
}

void biosal_assembly_connectivity_delete_child(struct biosal_assembly_connectivity *self, int symbol_code)
{
    biosal_bitmap_set_bit_value_uint8_t(&self->bitmap,
                biosal_assembly_connectivity_child_offset(symbol_code),
                    BIOSAL_BIT_ZERO);
}

int biosal_assembly_connectivity_get_element(struct biosal_assembly_connectivity *self, int index, int type)
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

        if (type == BIOSAL_ARC_TYPE_CHILD) {
            offset = biosal_assembly_connectivity_child_offset(i);

        } else if (type == BIOSAL_ARC_TYPE_PARENT) {

            offset = biosal_assembly_connectivity_parent_offset(i);
        }

        value = biosal_bitmap_get_bit_uint8_t(&self->bitmap, offset);

        if (value) {
            skipped++;
        }

        if (skipped == bits_to_skip) {
            return i;
        }
    }

    return -1;
}

int biosal_assembly_connectivity_get_child(struct biosal_assembly_connectivity *self, int index)
{
    return biosal_assembly_connectivity_get_element(self, index, BIOSAL_ARC_TYPE_CHILD);
}


int biosal_assembly_connectivity_parent_count(struct biosal_assembly_connectivity *self)
{
    return biosal_assembly_connectivity_get_count(self, BIOSAL_ARC_TYPE_PARENT);
}

void biosal_assembly_connectivity_add_parent(struct biosal_assembly_connectivity *self, int symbol_code)
{
    biosal_bitmap_set_bit_value_uint8_t(&self->bitmap,
                biosal_assembly_connectivity_parent_offset(symbol_code),
                    BIOSAL_BIT_ONE);
}

void biosal_assembly_connectivity_delete_parent(struct biosal_assembly_connectivity *self, int symbol_code)
{
    biosal_bitmap_set_bit_value_uint8_t(&self->bitmap,
                biosal_assembly_connectivity_parent_offset(symbol_code),
                    BIOSAL_BIT_ZERO);
}

int biosal_assembly_connectivity_get_parent(struct biosal_assembly_connectivity *self, int index)
{
    return biosal_assembly_connectivity_get_element(self, index, BIOSAL_ARC_TYPE_PARENT);
}

int biosal_assembly_connectivity_parent_offset(int code)
{
    return BIOSAL_ASSEMBLY_CONNECTIVITY_OFFSET_PARENTS + code;
}

int biosal_assembly_connectivity_child_offset(int code)
{
    return BIOSAL_ASSEMBLY_CONNECTIVITY_OFFSET_CHILDREN + code;
}

void biosal_assembly_connectivity_print(struct biosal_assembly_connectivity *self)
{
    int size;
    int code;
    int i;
    char nucleotide;

    size = biosal_assembly_connectivity_parent_count(self);

    printf("Parents: %d", size);

    printf(" [");

    for (i = 0; i < size; i++) {

        code = biosal_assembly_connectivity_get_parent(self, i);

        nucleotide = biosal_dna_codec_get_nucleotide_from_code(code);

        printf("%c", nucleotide);
    }

    printf("] ");

    size = biosal_assembly_connectivity_child_count(self);

    printf("Children: %d", size);

    printf(" [");

    for (i = 0; i < size; i++) {

        code = biosal_assembly_connectivity_get_child(self, i);

        nucleotide = biosal_dna_codec_get_nucleotide_from_code(code);

        printf("%c", nucleotide);
    }

    printf("]");
}

int biosal_assembly_connectivity_pack_size(struct biosal_assembly_connectivity *self)
{
    return biosal_assembly_connectivity_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK_SIZE, NULL);
}

int biosal_assembly_connectivity_pack(struct biosal_assembly_connectivity *self, void *buffer)
{
    return biosal_assembly_connectivity_pack_unpack(self, BIOSAL_PACKER_OPERATION_PACK, buffer);
}

int biosal_assembly_connectivity_unpack(struct biosal_assembly_connectivity *self, void *buffer)
{
    return biosal_assembly_connectivity_pack_unpack(self, BIOSAL_PACKER_OPERATION_UNPACK, buffer);
}

int biosal_assembly_connectivity_pack_unpack(struct biosal_assembly_connectivity *self, int operation, void *buffer)
{
    int bytes;
    struct biosal_packer packer;

    biosal_packer_init(&packer, operation, buffer);

    biosal_packer_process(&packer, &self->bitmap, sizeof(self->bitmap));

    bytes = biosal_packer_get_byte_count(&packer);

    biosal_packer_destroy(&packer);

    return bytes;
}

void biosal_assembly_connectivity_init_copy(struct biosal_assembly_connectivity *self,
                struct biosal_assembly_connectivity *connectivity)
{
    self->bitmap = connectivity->bitmap;
}

void biosal_assembly_connectivity_invert_arcs(struct biosal_assembly_connectivity *self)
{
    struct biosal_assembly_connectivity copy;
    int parents;
    int children;
    int i;
    int code;
    int new_code;

    biosal_assembly_connectivity_init_copy(&copy, self);

    /*
     * Remove all links
     */
    biosal_assembly_connectivity_destroy(self);
    biosal_assembly_connectivity_init(self);

    /*
     * Parents become children
     */
    parents = biosal_assembly_connectivity_parent_count(&copy);

    for (i = 0; i < parents; i++) {
        code = biosal_assembly_connectivity_get_parent(&copy, i);

        new_code = biosal_dna_codec_get_complement(code);

        biosal_assembly_connectivity_add_child(self, new_code);
    }

    /*
     * Children become parents.
     */
    children = biosal_assembly_connectivity_child_count(&copy);

    for (i = 0; i < children; i++) {
        code = biosal_assembly_connectivity_get_child(&copy, i);

        new_code = biosal_dna_codec_get_complement(code);

        biosal_assembly_connectivity_add_parent(self, new_code);
    }

    biosal_assembly_connectivity_destroy(&copy);
}
