
#ifndef BSAL_MIGRATION_H
#define BSAL_MIGRATION_H

struct bsal_pool;

struct thorium_migration {
    int actor_name;
    int old_worker;
    int new_worker;
};

void thorium_migration_init(struct thorium_migration *self, int actor_name,
                int old_worker, int new_worker);
void thorium_migration_destroy(struct thorium_migration *self);

int thorium_migration_get_actor(struct thorium_migration *self);
int thorium_migration_get_old_worker(struct thorium_migration *self);
int thorium_migration_get_new_worker(struct thorium_migration *self);


#endif
