
#ifndef CORE_COMMAND_H
#define CORE_COMMAND_H

extern char CORE_DEFAULT_OUTPUT[];

#define CORE_DEFAULT_KMER_LENGTH 49

int core_command_has_argument(int argc, char **argv, const char *argument);
char *core_command_get_output_directory(int argc, char **argv);
char *core_command_get_argument_value(int argc, char **argv, const char *argument);
int core_command_get_argument_value_int(int argc, char **argv, const char *argument);
int core_command_get_kmer_length(int argc, char **argv);

#endif
