
#include "table.h"

#include <stdio.h>

struct thorium_script table_script = {
    .identifier = SCRIPT_TABLE,
    .init = table_init,
    .destroy = table_destroy,
    .receive = table_receive,
    .size = sizeof(struct table),
    .name = "table"
};

void table_init(struct thorium_actor *actor)
{
    struct table *table1;

    table1 = (struct table *)thorium_actor_concrete_actor(actor);
    table1->done = 0;
    bsal_vector_init(&table1->spawners, sizeof(int));
}

void table_destroy(struct thorium_actor *actor)
{
    struct table *table1;

    table1 = (struct table *)thorium_actor_concrete_actor(actor);

    bsal_vector_destroy(&table1->spawners);
    printf("actor %d dies\n", thorium_actor_name(actor));
}

void table_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    int name;
    int remote;
    struct thorium_message spawn_message;
    int script;
    int new_actor;
    void *buffer;
    struct table *table1;

    table1 = (struct table *)thorium_actor_concrete_actor(actor);
    source = thorium_message_source(message);
    tag = thorium_message_tag(message);
    name = thorium_actor_name(actor);
    buffer = thorium_message_buffer(message);

    if (tag == ACTION_START) {
        printf("Actor %i receives ACTION_START from actor %i\n",
                        name,  source);

        bsal_vector_init(&table1->spawners, 0);
        bsal_vector_unpack(&table1->spawners, buffer);

        remote = bsal_vector_index_of(&table1->spawners, &name) + 1;
        remote %= bsal_vector_size(&table1->spawners);

        script = SCRIPT_TABLE;
        thorium_message_init(&spawn_message, ACTION_SPAWN, sizeof(script), &script);
        thorium_actor_send(actor, *(int *)bsal_vector_at(&table1->spawners, remote), &spawn_message);

        /*
        printf("sending notification\n");
        thorium_message_init(message, ACTION_TABLE_NOTIFY, 0, NULL);
        thorium_actor_send(actor, 0, message);
*/
    } else if (tag == ACTION_SPAWN_REPLY) {

        new_actor= *(int *)buffer;

        printf("Actor %i receives ACTION_SPAWN_REPLY from actor %i,"
                        " new actor is %d\n",
                        name,  source, new_actor);

        thorium_message_init(message, ACTION_ACTION_TABLE_DIE2, 0, NULL);
        thorium_actor_send(actor, new_actor, message);

        thorium_message_init(message, ACTION_TABLE_NOTIFY, 0, NULL);
        thorium_actor_send(actor, bsal_vector_at_as_int(&table1->spawners, 0), message);

    } else if (tag == ACTION_ACTION_TABLE_DIE2) {

        printf("Actor %i receives ACTION_ACTION_TABLE_DIE2 from actor %i\n",
                        name,  source);

        if (name < bsal_vector_size(&table1->spawners)) {
            return;
        }

        thorium_message_init(message, ACTION_STOP, 0, NULL);
        thorium_actor_send(actor, name, message);

    } else if (tag == ACTION_TABLE_DIE) {

        printf("Actor %i receives ACTION_TABLE_DIE from actor %i\n",
                        name,  source);

        thorium_message_init(message, ACTION_STOP, 0, NULL);
        thorium_actor_send(actor, name, message);

    } else if (tag == ACTION_TABLE_NOTIFY) {

        printf("Actor %i receives ACTION_TABLE_NOTIFY from actor %i\n",
                        name,  source);

        table1->done++;

        if (table1->done == bsal_vector_size(&table1->spawners)) {
            printf("actor %d kills %d to %d\n",
                           name, 0, (int)bsal_vector_size(&table1->spawners) - 1);
            thorium_message_init(message, ACTION_TABLE_DIE, 0, NULL);
            thorium_actor_send_range(actor, &table1->spawners, message);
        }
    }
}
