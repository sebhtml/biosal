
#include "frame.h"

#include <stdio.h>
#include <stdlib.h>

struct thorium_script frame_script = {
    .identifier = SCRIPT_FRAME,
    .init = frame_init,
    .destroy = frame_destroy,
    .receive = frame_receive,
    .size = sizeof(struct frame),
    .name = "frame"
};

void frame_init(struct thorium_actor *actor)
{
    struct frame *concrete_actor;

    concrete_actor = (struct frame *)thorium_actor_concrete_actor(actor);
    concrete_actor->value = rand() % 12345;

    thorium_actor_send_to_self_empty(actor, ACTION_PACK_ENABLE);

    concrete_actor->migrated_other = 0;
    concrete_actor->pings = 0;
    bsal_vector_init(&concrete_actor->acquaintance_vector, sizeof(int));
}

void frame_destroy(struct thorium_actor *actor)
{
    struct frame *concrete_actor;

    concrete_actor = (struct frame *)thorium_actor_concrete_actor(actor);
    concrete_actor->value = -1;

    bsal_vector_destroy(&concrete_actor->acquaintance_vector);
}

void frame_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int name;
    void *buffer;
    struct frame *concrete_actor;
    int other;
    struct bsal_vector initial_actors;
    struct bsal_vector *acquaintance_vector;
    int source;

    source = thorium_message_source(message);
    concrete_actor = (struct frame *)thorium_actor_concrete_actor(actor);
    tag = thorium_message_action(message);
    name = thorium_actor_name(actor);
    buffer = thorium_message_buffer(message);
    acquaintance_vector = &concrete_actor->acquaintance_vector;


    if (tag == ACTION_START) {

        bsal_vector_init(&initial_actors, sizeof(int));
        bsal_vector_unpack(&initial_actors, buffer);
        bsal_vector_push_back_vector(acquaintance_vector, &initial_actors);
        bsal_vector_destroy(&initial_actors);

        other = thorium_actor_spawn(actor, thorium_actor_script(actor));
        bsal_vector_push_back_int(acquaintance_vector, other);

        thorium_actor_send_empty(actor, other, ACTION_PING);

        printf("actor %d sends ACTION_PING to new actor %d\n",
                        name, other);

    } else if (tag == ACTION_PING) {

        /* new acquaintance
         */
        bsal_vector_push_back(acquaintance_vector, &source);

        printf("actor %d (value %d) receives ACTION_PING from actor %d\n",
                        name, concrete_actor->value, source);
        printf("Acquaintances of actor %d: ", name);
        bsal_vector_print_int(acquaintance_vector);
        printf("\n");

        thorium_actor_send_reply_empty(actor, ACTION_PING_REPLY);

    } else if (tag == ACTION_PING_REPLY) {

        concrete_actor->pings++;

        printf("actor %d receives ACTION_PING_REPLY from actor %d\n",
                        name, source);

        /* kill the system
         */
        if (concrete_actor->migrated_other && concrete_actor->pings == 2) {

            thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP);
            thorium_actor_send_to_self_empty(actor, ACTION_ASK_TO_STOP);

            return;
        }

        /* migrate other actor
         */

        printf("actor %d asks actor %d to migrate using actor %d as spawner\n",
                        name, source, name);

        printf("Acquaintances of actor %d: ", name);
        bsal_vector_print_int(acquaintance_vector);
        printf("\n");

        thorium_actor_send_reply_int(actor, ACTION_MIGRATE, name);

        /* send a message to other while it is migrating.
         * this is supposed to work !
         */
        printf("actor %d sends ACTION_PING to %d while it is migrating\n",
                        name, source);
        thorium_actor_send_reply_empty(actor, ACTION_PING);

        concrete_actor->migrated_other = 1;

    } else if (tag == ACTION_MIGRATE_REPLY) {

        thorium_message_unpack_int(message, 0, &other);
        printf("actor %d received migrated actor %d\n", name, other);
        printf("Acquaintances of actor %d: ", name);
        bsal_vector_print_int(acquaintance_vector);
        printf("\n");

        /* it is possible that the ACTION_PING went through
         * before the migration
         */
        if (concrete_actor->pings == 2) {
            thorium_actor_send_reply_empty(actor, ACTION_ASK_TO_STOP);
            thorium_actor_send_to_self_empty(actor, ACTION_ASK_TO_STOP);
        }

    } else if (tag == ACTION_PACK) {

        thorium_actor_send_reply_int(actor, ACTION_PACK_REPLY, concrete_actor->value);

    } else if (tag == ACTION_UNPACK) {

        thorium_message_unpack_int(message, 0, &concrete_actor->value);
        thorium_actor_send_reply_empty(actor, ACTION_UNPACK_REPLY);

    } else if (tag == ACTION_ASK_TO_STOP) {

        printf("actor %d received ACTION_ASK_TO_STOP, value: %d ",
                        name, concrete_actor->value);
        printf("acquaintance vector: ");
        bsal_vector_print_int(acquaintance_vector);
        printf("\n");

        thorium_actor_send_to_self_empty(actor, ACTION_STOP);
    }
}
