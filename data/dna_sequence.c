
#include "dna_sequence.h"

#include <stdlib.h>
#include <string.h>

void bsal_dna_sequence_init(struct bsal_dna_sequence *sequence, char *raw_data)
{
    /* encode @raw_data in 2-bit format
     * use an allocator provided to allocate memory
     */
    sequence->data = NULL;
    sequence->length = strlen(raw_data);
    sequence->pair = -1;
}

void bsal_dna_sequence_destroy(struct bsal_dna_sequence *sequence)
{
    sequence->data = NULL;
    sequence->length = 0;
    sequence->pair = -1;
}
