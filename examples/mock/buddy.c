
#include "buddy.h"

#include <stdio.h>

/* this script is required */
struct thorium_script buddy_script = {
    .identifier = BUDDY_SCRIPT,
    .init = buddy_init,
    .destroy = buddy_destroy,
    .receive = buddy_receive,
    .size = sizeof(struct buddy),
    .name = "buddy"
};

void buddy_init(struct thorium_actor *actor)
{
    struct buddy *buddy1;

    buddy1 = (struct buddy *)thorium_actor_concrete_actor(actor);
    buddy1->received = 0;
}

void buddy_destroy(struct thorium_actor *actor)
{
    struct buddy *buddy1;

    buddy1 = (struct buddy *)thorium_actor_concrete_actor(actor);
    buddy1->received = -1;
}

void buddy_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int source;
    int name;

    name = thorium_actor_name(actor);
    source = thorium_message_source(message);
    tag = thorium_message_tag(message);

    if (tag == BUDDY_BOOT) {

        printf("BUDDY_BOOT\n");
        thorium_actor_print(actor);

        thorium_message_init(message, BUDDY_BOOT_REPLY, 0, NULL);
        thorium_actor_send(actor, source, message);

    } else if (tag == BUDDY_HELLO) {

        printf("BUDDY_HELLO\n");

        /* pin the actor to the worker for no reason !
         */

        /*
        thorium_actor_send_to_self_empty(actor, BSAL_ACTOR_PIN_TO_WORKER);
        */

        thorium_message_init(message, BUDDY_HELLO_REPLY, 0, NULL);
        thorium_actor_send(actor, source, message);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("BUDDY_DIE\n");

        printf("buddy_receive Actor %i received a message (%i BUDDY_DIE) from actor %i\n",
                        name, tag, source);

        /*
        thorium_actor_send_to_self_empty(actor, BSAL_ACTOR_UNPIN_FROM_WORKER);
        */

        thorium_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
        thorium_actor_send(actor, name, message);
    }
}
