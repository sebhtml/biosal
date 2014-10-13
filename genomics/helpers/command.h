
#ifndef BIOSAL_COMMAND_H
#define BIOSAL_COMMAND_H

extern char BIOSAL_DEFAULT_OUTPUT[];

#define BIOSAL_DEFAULT_KMER_LENGTH 49

int biosal_command_get_kmer_length(int argc, char **argv);
char *biosal_command_get_output_directory(int argc, char **argv);
int biosal_command_get_minimum_coverage(int argc, char **argv);

#endif
