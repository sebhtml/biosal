
#include "table.h"

#include <stdio.h>

struct bsal_script table_script = {
    .name = TABLE_SCRIPT,
    .init = table_init,
    .destroy = table_destroy,
    .receive = table_receive,
    .size = sizeof(struct table)
};

void table_init(struct bsal_actor *actor)
{
    struct table *table1;

    table1 = (struct table *)bsal_actor_concrete_actor(actor);
    table1->done = 0;
    bsal_vector_init(&table1->spawners, sizeof(int));
}

void table_destroy(struct bsal_actor *actor)
{
    struct table *table1;

    table1 = (struct table *)bsal_actor_concrete_actor(actor);

    bsal_vector_destroy(&table1->spawners);
    printf("actor %d dies\n", bsal_actor_name(actor));
}

void table_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;
    int remote;
    struct bsal_message spawn_message;
    int script;
    int new_actor;
    void *buffer;
    struct table *table1;

    table1 = (struct table *)bsal_actor_concrete_actor(actor);
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);
    name = bsal_actor_name(actor);
    buffer = bsal_message_buffer(message);

    if (tag == BSAL_ACTOR_START) {
        printf("Actor %i receives BSAL_ACTOR_START from actor %i\n",
                        name,  source);

        bsal_vector_unpack(&table1->spawners, buffer);

        remote = name + 1;
        remote %= bsal_vector_size(&table1->spawners);

        script = TABLE_SCRIPT;
        bsal_message_init(&spawn_message, BSAL_ACTOR_SPAWN, sizeof(script), &script);
        bsal_actor_send(actor, remote, &spawn_message);

        /*
        printf("sending notification\n");
        bsal_message_init(message, TABLE_NOTIFY, 0, NULL);
        bsal_actor_send(actor, 0, message);
*/
    } else if (tag == BSAL_ACTOR_SPAWN_REPLY) {

        new_actor= *(int *)buffer;

        printf("Actor %i receives BSAL_ACTOR_SPAWN_REPLY from actor %i,"
                        " new actor is %d\n",
                        name,  source, new_actor);

        bsal_message_init(message, TABLE_DIE2, 0, NULL);
        bsal_actor_send(actor, new_actor, message);

        bsal_message_init(message, TABLE_NOTIFY, 0, NULL);
        bsal_actor_send(actor, 0, message);

    } else if (tag == TABLE_DIE2) {

        printf("Actor %i receives TABLE_DIE2 from actor %i\n",
                        name,  source);

        if (name < bsal_vector_size(&table1->spawners)) {
            return;
        }

        bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
        bsal_actor_send(actor, name, message);

    } else if (tag == TABLE_DIE) {

        printf("Actor %i receives TABLE_DIE from actor %i\n",
                        name,  source);

        bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
        bsal_actor_send(actor, name, message);

    } else if (tag == TABLE_NOTIFY) {

        printf("Actor %i receives TABLE_NOTIFY from actor %i\n",
                        name,  source);

        table1->done++;

        if (table1->done == bsal_vector_size(&table1->spawners)) {
            printf("actor %d kills %d to %d\n",
                           name, 0, bsal_vector_size(&table1->spawners) - 1);
            bsal_message_init(message, TABLE_DIE, 0, NULL);
            bsal_actor_send_range_standard(actor, 0, bsal_vector_size(&table1->spawners) - 1, message);
        }
    }
}
