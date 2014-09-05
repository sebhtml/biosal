
#ifndef BSAL_DNA_HELPER_H
#define BSAL_DNA_HELPER_H

void bsal_dna_helper_reverse_complement_in_place(char *sequence);
char bsal_dna_helper_complement_nucleotide(char nucleotide);
void bsal_dna_helper_reverse_complement(char *sequence1, char *sequence2);
void bsal_dna_helper_normalize(char *sequence);
char bsal_dna_helper_normalize_nucleotide(char nucleotide);

void bsal_dna_helper_set_lower_case(char *sequence, int start, int end);

#endif
