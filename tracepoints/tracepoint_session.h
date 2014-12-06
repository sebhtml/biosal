
#ifndef THORIUM_TRACEPOINT_SESSION_H
#define THORIUM_TRACEPOINT_SESSION_H

#include <core/structures/vector.h>

/*
 * A tracepoint session.
 */
struct thorium_tracepoint_session {
    struct core_vector names;
    struct core_vector values;
};

void thorium_tracepoint_session_init(struct thorium_tracepoint_session *self);
void thorium_tracepoint_session_destroy(struct thorium_tracepoint_session *self);

void thorium_tracepoint_session_add_tracepoint(struct thorium_tracepoint_session *self,
                int tracepoint, const char *name);
void thorium_tracepoint_session_set_tracepoint_time(struct thorium_tracepoint_session *self,
                int tracepoint, uint64_t time);
uint64_t thorium_tracepoint_session_get_tracepoint_time(struct thorium_tracepoint_session *self,
                int tracepoint);
#endif
