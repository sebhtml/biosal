
#include "log.h"

#include <engine/thorium/actor.h>

#include <stdio.h>

void thorium_actor_log(struct thorium_actor *self, const char *format, ...)
{
    va_list arguments;
    FILE *stream;
    char *script_name;
    int name;

    name = thorium_actor_name(self);
    script_name = thorium_actor_script_name(self);

    stream = stdout;

    /*
     * \see http://stackoverflow.com/questions/1516370/wrapper-printf-function-that-filters-according-to-user-preferences
     */

    va_start(arguments, format);

    fprintf(stream, "[ACTOR_LOG] TIME %s %d ... ",
                    script_name, name);

    vfprintf(stream, format, arguments);

    fprintf(stream, "\n");

    fflush(stream);

    va_end(arguments);
}
