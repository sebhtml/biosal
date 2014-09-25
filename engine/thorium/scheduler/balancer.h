
#ifndef THORIUM_SCHEDULER_H
#define THORIUM_SCHEDULER_H

#include <core/structures/map.h>

struct bsal_vector;
struct thorium_worker_pool;
struct thorium_message;
struct thorium_actor;
struct thorium_migration;

#define THORIUM_SCHEDULER_PERIOD_IN_SECONDS 30

struct thorium_balancer {
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

void thorium_balancer_init(struct thorium_balancer *self, struct thorium_worker_pool *pool);
void thorium_balancer_destroy(struct thorium_balancer *self);

void thorium_balancer_balance(struct thorium_balancer *self);
void thorium_balancer_migrate(struct thorium_balancer *self, struct thorium_migration *migration);

int thorium_balancer_get_actor_production(struct thorium_balancer *self, struct thorium_actor *actor);
void thorium_balancer_update_actor_production(struct thorium_balancer *self, struct thorium_actor *actor);
int thorium_balancer_get_actor_worker(struct thorium_balancer *self, int name);
void thorium_balancer_set_actor_worker(struct thorium_balancer *self, int name, int worker_index);

int thorium_balancer_select_worker_least_busy(
                struct thorium_balancer *self, int *worker_score);
void thorium_balancer_detect_symmetric_scripts(struct thorium_balancer *self, struct bsal_map *symmetric_actors);
void thorium_balancer_generate_symmetric_migrations(struct thorium_balancer *self, struct bsal_map *symmetric_actor_scripts,
                struct bsal_vector *migrations);

int thorium_balancer_select_worker_script_round_robin(struct thorium_balancer *self, int script);

#endif
