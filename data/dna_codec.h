
#ifndef BSAL_DNA_CODEC_H
#define BSAL_DNA_CODEC_H

int bsal_dna_codec_encoded_length(int length_in_nucleotides);
void bsal_dna_codec_encode(int length_in_nucleotides, void *dna_sequence, void *encoded_sequence);
void bsal_dna_codec_decode(int length_in_nucleotides, void *encoded_sequence, void *dna_sequence);

#endif
