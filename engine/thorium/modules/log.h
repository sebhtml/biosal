
#ifndef ACTOR_LOG_H
#define ACTOR_LOG_H

#include <stdarg.h>

#define THORIUM_ACTOR_LOG_ACTION_BASE (-18000)

#define ACTION_ENABLE_DEFAULT_LOG_LEVEL (THORIUM_ACTOR_LOG_ACTION_BASE + 1)
#define ACTION_DISABLE_DEFAULT_LOG_LEVEL (THORIUM_ACTOR_LOG_ACTION_BASE + 2)

struct thorium_actor;

void thorium_actor_log(struct thorium_actor *self, const char *format, ...);

#endif
