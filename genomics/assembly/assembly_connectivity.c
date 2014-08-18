
#include "assembly_connectivity.h"

#include "assembly_arc.h"

#include <core/helpers/bitmap.h>
#include <core/system/packer.h>

#include <genomics/data/dna_codec.h>

#include <stdio.h>

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

        value = bsal_bitmap_get_bit_uint8_t(&self->bitmap,
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

        value = bsal_bitmap_get_bit_uint8_t(&self->bitmap, offset);

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

void bsal_assembly_connectivity_print(struct bsal_assembly_connectivity *self)
{
    int size;
    int code;
    int i;
    char nucleotide;

    size = bsal_assembly_connectivity_parent_count(self);

    printf("Parents: %d", size);

    printf(" [");

    for (i = 0; i < size; i++) {

        code = bsal_assembly_connectivity_get_parent(self, i);

        nucleotide = bsal_dna_codec_get_nucleotide_from_code(code);

        printf("%c", nucleotide);
    }

    printf("] ");

    size = bsal_assembly_connectivity_child_count(self);

    printf("Children: %d", size);

    printf(" [");

    for (i = 0; i < size; i++) {

        code = bsal_assembly_connectivity_get_child(self, i);

        nucleotide = bsal_dna_codec_get_nucleotide_from_code(code);

        printf("%c", nucleotide);
    }

    printf("]");
}

int bsal_assembly_connectivity_pack_size(struct bsal_assembly_connectivity *self)
{
    return bsal_assembly_connectivity_pack_unpack(self, BSAL_PACKER_OPERATION_DRY_RUN, NULL);
}

int bsal_assembly_connectivity_pack(struct bsal_assembly_connectivity *self, void *buffer)
{
    return bsal_assembly_connectivity_pack_unpack(self, BSAL_PACKER_OPERATION_PACK, buffer);
}

int bsal_assembly_connectivity_unpack(struct bsal_assembly_connectivity *self, void *buffer)
{
    return bsal_assembly_connectivity_pack_unpack(self, BSAL_PACKER_OPERATION_UNPACK, buffer);
}

int bsal_assembly_connectivity_pack_unpack(struct bsal_assembly_connectivity *self, int operation, void *buffer)
{
    int bytes;
    struct bsal_packer packer;

    bsal_packer_init(&packer, operation, buffer);

    bsal_packer_work(&packer, &self->bitmap, sizeof(self->bitmap));

    bytes = bsal_packer_worked_bytes(&packer);

    bsal_packer_destroy(&packer);

    return bytes;
}

void bsal_assembly_connectivity_init_copy(struct bsal_assembly_connectivity *self,
                struct bsal_assembly_connectivity *connectivity)
{
    self->bitmap = connectivity->bitmap;
}

void bsal_assembly_connectivity_invert_arcs(struct bsal_assembly_connectivity *self)
{
    struct bsal_assembly_connectivity copy;
    int parents;
    int children;
    int i;
    int code;
    int new_code;

    bsal_assembly_connectivity_init_copy(&copy, self);

    /*
     * Remove all links
     */
    bsal_assembly_connectivity_destroy(self);
    bsal_assembly_connectivity_init(self);

    /*
     * Parents become children
     */
    parents = bsal_assembly_connectivity_parent_count(&copy);

    for (i = 0; i < parents; i++) {
        code = bsal_assembly_connectivity_get_parent(&copy, i);

        new_code = bsal_dna_codec_get_complement(code);

        bsal_assembly_connectivity_add_child(self, new_code);
    }

    /*
     * Children become parents.
     */
    children = bsal_assembly_connectivity_child_count(&copy);

    for (i = 0; i < children; i++) {
        code = bsal_assembly_connectivity_get_child(&copy, i);

        new_code = bsal_dna_codec_get_complement(code);

        bsal_assembly_connectivity_add_parent(self, new_code);
    }

    bsal_assembly_connectivity_destroy(&copy);
}
