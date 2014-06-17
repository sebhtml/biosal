
#include "dna_codec.h"

#include <string.h>

int bsal_dna_codec_encoded_length(int length_in_nucleotides)
{
    return length_in_nucleotides + 1;
}

void bsal_dna_codec_encode(int length_in_nucleotides, void *dna_sequence, void *encoded_sequence)
{
    memcpy(encoded_sequence, dna_sequence, length_in_nucleotides);
    ((char *)encoded_sequence)[length_in_nucleotides] = '\0';
}

void bsal_dna_codec_decode(int length_in_nucleotides, void *encoded_sequence, void *dna_sequence)
{
    memcpy(dna_sequence, encoded_sequence, length_in_nucleotides);
    ((char *)dna_sequence)[length_in_nucleotides] = '\0';
}


