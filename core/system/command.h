
#ifndef CORE_COMMAND_H
#define CORE_COMMAND_H

int core_command_has_argument(int argc, char **argv, const char *argument);
char *core_command_get_argument_value(int argc, char **argv, const char *argument);
int core_command_get_argument_value_int(int argc, char **argv, const char *argument);

#endif
