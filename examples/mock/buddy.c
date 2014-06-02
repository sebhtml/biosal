
#include "buddy.h"

#include <stdio.h>

/* this script is required */
struct bsal_script buddy_script = {
    .name = BUDDY_SCRIPT,
    .init = buddy_init,
    .destroy = buddy_destroy,
    .receive = buddy_receive,
    .size = sizeof(struct buddy)
};

void buddy_init(struct bsal_actor *actor)
{
    struct buddy *buddy1;

    buddy1 = (struct buddy *)bsal_actor_state(actor);
    buddy1->received = 0;
}

void buddy_destroy(struct bsal_actor *actor)
{
    struct buddy *buddy1;

    buddy1 = (struct buddy *)bsal_actor_state(actor);
    buddy1->received = -1;
}

void buddy_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int source;
    int name;

    name = bsal_actor_name(actor);
    source = bsal_message_source(message);
    tag = bsal_message_tag(message);

    if (tag == BUDDY_BOOT) {

        printf("BUDDY_BOOT\n");
        buddy_init(actor);
        bsal_actor_print(actor);

        bsal_message_set_tag(message, BUDDY_BOOT_OK);
        bsal_actor_send(actor, source, message);

    } else if (tag == BUDDY_HELLO) {

        printf("BUDDY_HELLO\n");

        /* pin the actor to the worker for no reason !
         */
        bsal_message_set_tag(message, BSAL_ACTOR_PIN);
        bsal_actor_send(actor, name, message);

        bsal_message_set_tag(message, BUDDY_HELLO_OK);
        bsal_actor_send(actor, source, message);

    } else if (tag == BUDDY_DIE) {
        printf("BUDDY_DIE\n");

        printf("buddy_receive Actor %i received a message (%i BUDDY_DIE) from actor %i\n",
                        name, tag, source);

        bsal_message_set_tag(message, BSAL_ACTOR_UNPIN);
        bsal_actor_send(actor, name, message);

        bsal_message_set_tag(message, BSAL_ACTOR_STOP);
        bsal_actor_send(actor, name, message);
    }
}
