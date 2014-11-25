
#ifndef ACTOR_LOG_H
#define ACTOR_LOG_H

#include <stdarg.h>

struct thorium_actor;

void thorium_actor_log(struct thorium_actor *self, const char *format, ...);

#endif
