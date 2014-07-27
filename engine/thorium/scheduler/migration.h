
#ifndef BSAL_MIGRATION_H
#define BSAL_MIGRATION_H

struct bsal_pool;

struct bsal_migration {
    int actor_name;
    int old_worker;
    int new_worker;
};

void bsal_migration_init(struct bsal_migration *migration, int actor_name,
                int old_worker, int new_worker);
void bsal_migration_destroy(struct bsal_migration *migration);

int bsal_migration_get_actor(struct bsal_migration *migration);
int bsal_migration_get_old_worker(struct bsal_migration *migration);
int bsal_migration_get_new_worker(struct bsal_migration *migration);


#endif
