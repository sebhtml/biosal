
#ifndef BIOSAL_COMMAND_H
#define BIOSAL_COMMAND_H

extern char BIOSAL_DEFAULT_OUTPUT[];

#define BIOSAL_DEFAULT_KMER_LENGTH 49

#define BIOSAL_DEFAULT_MINIMUM_COVERAGE 2

int biosal_command_get_kmer_length(int argc, char **argv);
char *biosal_command_get_output_directory(int argc, char **argv);

/*
 * The minimum coverage depth. Anything with a coverage depth
 * below that threshold will be ignored.
 *
 * Implementation:
 *
 * 1. graph store actors skip such vertices
 * 2. visitor actors use a heuristic which ignores such vertices.
 * 3. walker actors strictly rely on the BIOSAL_VERTEX_FLAG_UNITIG.
 */
int biosal_command_get_minimum_coverage(int argc, char **argv);

#endif
