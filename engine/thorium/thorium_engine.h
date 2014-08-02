
#ifndef BSAL_THORIUM_ENGINE_H
#define BSAL_THORIUM_ENGINE_H

#include "node.h"
#include "actor.h"
#include "message.h"

struct bsal_script;

/*
 * The Thorium engine is a distributed actor machine
 * implementation.
 */
struct bsal_thorium_engine {
    struct bsal_node node;
};

void bsal_thorium_engine_init(struct bsal_thorium_engine *self, int *argc, char ***argv);
void bsal_thorium_engine_destroy(struct bsal_thorium_engine *self);

int bsal_thorium_engine_boot_initial_actor(struct bsal_thorium_engine *self, int script_identifier,
                struct bsal_script *script);

/*
 * Use this function in main() to boot an actor system.
 */
int bsal_thorium_boot_initial_actor(int *argc, char ***argv, int script_identifier, struct bsal_script *script);

#endif
