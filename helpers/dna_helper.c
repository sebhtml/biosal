
#include "dna_helper.h"

#include <string.h>

void bsal_dna_helper_reverse_complement_in_place(char *sequence)
{
    int position;
    int length;
    int middle;
    char saved;
    int position2;

    length = strlen(sequence);
    middle = length / 2;

    position = 0;

    while (position < length) {
        sequence[position] = bsal_dna_helper_complement_nucleotide(sequence[position]);
        position++;
    }

    position = 0;

    while (position < middle) {
        position2 = length - 1 - position;
        saved = sequence[position2];
        sequence[position2] = sequence[position];
        sequence[position] = saved;
        position++;
    }
}

void bsal_dna_helper_reverse_complement(char *sequence1, char *sequence2)
{
    int length;
    int position1;
    int position2;
    char nucleotide1;
    char nucleotide2;

    length = strlen(sequence1);

    position1 = length - 1;
    position2 = 0;

    while (position2 < length) {

        nucleotide1 = sequence1[position1];
        nucleotide2 = bsal_dna_helper_complement_nucleotide(nucleotide1);

        sequence2[position2] = nucleotide2;

        position2++;
        position1--;
    }

    sequence2[position2] = '\0';

#ifdef BSAL_DNA_KMER_DEBUG
    printf("%s and %s\n", sequence1, sequence2);
#endif
}

#define BSAL_DNA_HELPER_FAST

char bsal_dna_helper_complement_nucleotide(char nucleotide)
{
#ifdef BSAL_DNA_HELPER_FAST
    if (nucleotide == 'A') {
        return 'T';
    } else if (nucleotide == 'G') {
        return 'C';
    } else if (nucleotide == 'T') {
        return 'A';
    } else /* if (nucleotide == 'C') */ {
        return 'G';
    }
    return nucleotide;
#else
    switch (nucleotide) {
        case 'A':
            return 'T';
        case 'T':
            return 'A';
        case 'C':
            return 'G';
        case 'G':
            return 'C';
        default:
            return nucleotide;
    }

    return nucleotide;
#endif
}

void bsal_dna_helper_normalize(char *sequence)
{
    int length;
    int position;

    length = strlen(sequence);

    position = 0;

    while (position < length) {

        sequence[position] = bsal_dna_helper_normalize_nucleotide(sequence[position]);

        position++;
    }
}

char bsal_dna_helper_normalize_nucleotide(char nucleotide)
{
    char default_value;

    default_value = 'A';

    switch (nucleotide) {
        case 't':
            return 'T';
        case 'a':
            return 'A';
        case 'g':
            return 'G';
        case 'c':
            return 'C';
        case 'T':
            return 'T';
        case 'A':
            return 'A';
        case 'G':
            return 'G';
        case 'C':
            return 'C';
        case 'n':
            return default_value;
        case '.':
            return default_value;
        default:
            return default_value;
    }

    return default_value;
}


