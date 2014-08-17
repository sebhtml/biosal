
#include "migration.h"

#include <engine/thorium/worker_pool.h>
#include <engine/thorium/worker.h>

void thorium_migration_init(struct thorium_migration *migration, int actor_name,
                int old_worker, int new_worker)
{
    migration->actor_name = actor_name;
    migration->old_worker = old_worker;
    migration->new_worker = new_worker;
}

void thorium_migration_destroy(struct thorium_migration *migration)
{
    migration->actor_name = -1;
    migration->old_worker = -1;
    migration->new_worker = -1;
}


int thorium_migration_get_actor(struct thorium_migration *migration)
{
    return migration->actor_name;
}

int thorium_migration_get_old_worker(struct thorium_migration *migration)
{
    return migration->old_worker;
}

int thorium_migration_get_new_worker(struct thorium_migration *migration)
{
    return migration->new_worker;
}
