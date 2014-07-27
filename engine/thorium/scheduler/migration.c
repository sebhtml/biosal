
#include "migration.h"

#include <engine/thorium/worker_pool.h>
#include <engine/thorium/worker.h>

void bsal_migration_init(struct bsal_migration *migration, int actor_name,
                int old_worker, int new_worker)
{
    migration->actor_name = actor_name;
    migration->old_worker = old_worker;
    migration->new_worker = new_worker;
}

void bsal_migration_destroy(struct bsal_migration *migration)
{
    migration->actor_name = -1;
    migration->old_worker = -1;
    migration->new_worker = -1;
}


int bsal_migration_get_actor(struct bsal_migration *migration)
{
    return migration->actor_name;
}

int bsal_migration_get_old_worker(struct bsal_migration *migration)
{
    return migration->old_worker;
}

int bsal_migration_get_new_worker(struct bsal_migration *migration)
{
    return migration->new_worker;
}
