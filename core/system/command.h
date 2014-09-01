
#ifndef BSAL_COMMAND_H
#define BSAL_COMMAND_H

extern char BSAL_DEFAULT_OUTPUT[];

#define BSAL_DEFAULT_KMER_LENGTH 49

int bsal_command_has_argument(int argc, char **argv, const char *argument);
char *bsal_command_get_output_directory(int argc, char **argv);
char *bsal_command_get_argument_value(int argc, char **argv, const char *argument);
int bsal_command_get_argument_value_int(int argc, char **argv, const char *argument);
int bsal_command_get_kmer_length(int argc, char **argv);

#endif
