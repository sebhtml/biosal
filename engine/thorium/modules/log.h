
#ifndef THORIUM_ACTOR_LOG_H
#define THORIUM_ACTOR_LOG_H

#include "nop.h"

#include <stdarg.h>

#define THORIUM_ACTOR_LOG_ACTION_BASE (-18000)

#define ACTION_ENABLE_LOG_LEVEL (THORIUM_ACTOR_LOG_ACTION_BASE + 1)
#define ACTION_DISABLE_LOG_LEVEL (THORIUM_ACTOR_LOG_ACTION_BASE + 2)

#define LOG_LEVEL_DEFAULT THORIUM_ACTOR_FLAG_DEFAULT_LOG_LEVEL

struct thorium_actor;
struct thorium_message;

/*
 * Enables:
 *
 * - thorium_actor_log_with_level
 * - thorium_actor_log (thorium_actor_log_with_level with LOG_LEVEL_DEFAULT)
 */
/*
*/
#define THORIUM_ACTOR_LOG_ENABLED

#ifdef THORIUM_ACTOR_LOG_ENABLED

#define thorium_actor_log \
        thorium_actor_log_implementation

#define thorium_actor_log_with_level \
        thorium_actor_log_with_level_implementation

#else

#define thorium_actor_log(actor, format, ...) \
    thorium_nop(actor)

#define thorium_actor_log_with_level(actor, level, format, ...) \
    thorium_nop(actor)

#endif

#define LOG(...) \
        thorium_actor_log(self, __VA_ARGS__)

void thorium_actor_log_implementation(struct thorium_actor *self, const char *format, ...);
void thorium_actor_log_with_level_implementation(struct thorium_actor *self, int level, const char *format, ...);

void thorium_actor_enable_log_level(struct thorium_actor *self, struct thorium_message *message);
void thorium_actor_disable_log_level(struct thorium_actor *self, struct thorium_message *message);

#endif
