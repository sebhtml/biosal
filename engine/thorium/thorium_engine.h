
#ifndef THORIUM_THORIUM_ENGINE_H
#define THORIUM_THORIUM_ENGINE_H

#include "node.h"
#include "actor.h"
#include "message.h"

struct thorium_script;

/*
 * The Thorium engine is a distributed actor machine
 * implementation.
 */
struct biosal_thorium_engine {
    struct thorium_node node;
};

void biosal_thorium_engine_init(struct biosal_thorium_engine *self, int *argc, char ***argv);
void biosal_thorium_engine_destroy(struct biosal_thorium_engine *self);

int biosal_thorium_engine_boot(struct biosal_thorium_engine *self, int script_identifier,
                struct thorium_script *script);

/*
 * Use this function in main() to boot an actor system.
 *
 * The script_identifier can be obtained from the script, but it is easier to
 * read if the script_identifier is provided.
 */
int biosal_thorium_engine_boot_initial_actor(int *argc, char ***argv, int script_identifier, struct thorium_script *script);

#endif
