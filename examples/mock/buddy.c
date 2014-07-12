
#include "buddy.h"

#include <stdio.h>

/* this script is required */
struct bsal_script buddy_script = {
    .name = BUDDY_SCRIPT,
    .init = buddy_init,
    .destroy = buddy_destroy,
    .receive = buddy_receive,
    .size = sizeof(struct buddy),
    .description = "buddy"
};

void buddy_init(struct bsal_actor *actor)
{
    struct buddy *buddy1;

    buddy1 = (struct buddy *)bsal_actor_concrete_actor(actor);
    buddy1->received = 0;
}

void buddy_destroy(struct bsal_actor *actor)
{
    struct buddy *buddy1;

    buddy1 = (struct buddy *)bsal_actor_concrete_actor(actor);
    buddy1->received = -1;
}

void buddy_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;

    name = bsal_actor_get_name(actor);
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);

    if (tag == BUDDY_BOOT) {

        printf("BUDDY_BOOT\n");
        bsal_actor_print(actor);

        bsal_message_init(message, BUDDY_BOOT_REPLY, 0, NULL);
        bsal_actor_send(actor, source, message);

    } else if (tag == BUDDY_HELLO) {

        printf("BUDDY_HELLO\n");

        /* pin the actor to the worker for no reason !
         */
        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_PIN_TO_WORKER);

        bsal_message_init(message, BUDDY_HELLO_REPLY, 0, NULL);
        bsal_actor_send(actor, source, message);

    } else if (tag == BSAL_ACTOR_ASK_TO_STOP) {

        printf("BUDDY_DIE\n");

        printf("buddy_receive Actor %i received a message (%i BUDDY_DIE) from actor %i\n",
                        name, tag, source);

        bsal_actor_helper_send_to_self_empty(actor, BSAL_ACTOR_UNPIN_FROM_WORKER);

        bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
        bsal_actor_send(actor, name, message);
    }
}
