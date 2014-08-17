
#ifndef BSAL_SCHEDULER_H
#define BSAL_SCHEDULER_H

#include <core/structures/map.h>

struct bsal_vector;
struct thorium_worker_pool;
struct thorium_message;
struct thorium_actor;
struct thorium_migration;

#define BSAL_SCHEDULER_PERIOD_IN_SECONDS 30

struct thorium_scheduler {
    struct thorium_worker_pool *pool;
    struct bsal_map actor_affinities;
    struct bsal_map last_actor_received_messages;

    int worker_for_work;

    int last_migrations;
    int last_spawned_actors;
    int last_killed_actors;

    /*
     * For initial placement using round-robin policy for every script.
     */

    struct bsal_map current_script_workers;
    int first_worker;
};

void thorium_scheduler_init(struct thorium_scheduler *self, struct thorium_worker_pool *pool);
void thorium_scheduler_destroy(struct thorium_scheduler *self);

void thorium_scheduler_balance(struct thorium_scheduler *self);
void thorium_scheduler_migrate(struct thorium_scheduler *self, struct thorium_migration *migration);

int thorium_scheduler_get_actor_production(struct thorium_scheduler *self, struct thorium_actor *actor);
void thorium_scheduler_update_actor_production(struct thorium_scheduler *self, struct thorium_actor *actor);
int thorium_scheduler_get_actor_worker(struct thorium_scheduler *self, int name);
void thorium_scheduler_set_actor_worker(struct thorium_scheduler *self, int name, int worker_index);

int thorium_scheduler_select_worker_least_busy(
                struct thorium_scheduler *self, int *worker_score);
void thorium_scheduler_detect_symmetric_scripts(struct thorium_scheduler *self, struct bsal_map *symmetric_actors);
void thorium_scheduler_generate_symmetric_migrations(struct thorium_scheduler *self, struct bsal_map *symmetric_actor_scripts,
                struct bsal_vector *migrations);

int thorium_scheduler_select_worker_script_round_robin(struct thorium_scheduler *self, int script);

#endif
