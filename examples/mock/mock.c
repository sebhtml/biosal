
#include "mock.h"
#include "buddy.h"

#include <stdio.h>

struct bsal_script mock_script = {
    .name = MOCK_SCRIPT,
    .init = mock_init,
    .destroy = mock_destroy,
    .receive = mock_receive,
    .size = sizeof(struct mock)
};

void mock_init(struct bsal_actor *actor)
{
    struct mock *mock1;

    mock1 = (struct mock *)bsal_actor_concrete_actor(actor);
    mock1->value = 42;
    mock1->children[0] = -1;
    mock1->children[1] = -1;
    mock1->children[2] = -1;
    mock1->notified = 0;

    bsal_vector_init(&mock1->spawners, sizeof(int));
}

void mock_destroy(struct bsal_actor *actor)
{
    struct mock *mock1;

    mock1 = (struct mock *)bsal_actor_concrete_actor(actor);
    mock1->value = -1;

    bsal_vector_destroy(&mock1->spawners);
}

void mock_receive(struct bsal_actor *actor, struct bsal_message *message)
{
    int tag;
    int name;
    struct mock *mock1;
    char *buffer;

    mock1 = (struct mock *)bsal_actor_concrete_actor(actor);

    tag = bsal_message_tag(message);
    buffer = bsal_message_buffer(message);
    /*bsal_actor_print(actor);*/

    if (tag == BSAL_ACTOR_START) {

        bsal_vector_unpack(&mock1->spawners, buffer);

        printf("BSAL_ACTOR_START spawners: %d\n",
                        bsal_vector_size(&mock1->spawners));

        bsal_actor_add_script(actor, BUDDY_SCRIPT, &buddy_script);

        /*mock_init(actor);*/
        mock_start(actor, message);

    } else if (tag == MOCK_DIE) {
        printf("MOCK_DIE\n");
        mock_die(actor, message);

    } else if (tag == MOCK_NEW_CONTACTS) {
        printf("MOCK_NEW_CONTACTS\n");
        mock_add_contacts(actor, message);

    } else if (tag == MOCK_NEW_CONTACTS_OK) {

    } else if (tag == BUDDY_HELLO_OK) {

        printf("BUDDY_HELLO_OK\n");
        bsal_message_init(message, MOCK_NOTIFY, 0, NULL);
        bsal_actor_send(actor, 0, message);

    } else if (tag == MOCK_NOTIFY) {

        mock1->notified++;

        printf("notifications %d/%d\n", mock1->notified, bsal_vector_size(&mock1->spawners));

        if (mock1->notified == bsal_vector_size(&mock1->spawners)) {
            bsal_message_init(message, MOCK_PREPARE_DEATH, 0, NULL);

            /* the default binomial-tree algorithm can not
             * be used here because proxy actors may die
             * before they are needed.
             */
            bsal_actor_send_range_standard(actor, 0, bsal_vector_size(&mock1->spawners) - 1, message);

            printf("Stopping all initial actors now\n");
        }

    } else if (tag == MOCK_PREPARE_DEATH) {
        bsal_message_init(message, MOCK_DIE, 0, NULL);

        name = bsal_actor_name(actor);
        bsal_actor_send(actor, name, message);

        bsal_message_init(message, BUDDY_DIE, 0, NULL);
        bsal_actor_send(actor, mock1->children[0], message);

    } else if (tag == BUDDY_BOOT_OK) {

        mock_share(actor, message);
    }
}

void mock_add_contacts(struct bsal_actor *actor, struct bsal_message *message)
{
    int source;
    char *buffer;
    struct mock *mock1;

    mock1 = (struct mock *)bsal_actor_concrete_actor(actor);
    source = bsal_message_source(message);
    buffer = bsal_message_buffer(message);
    mock1->remote_actor = ((int*)buffer)[0];

    printf("mock_receive remote friend is %i\n",
                        mock1->remote_actor);

    bsal_message_init(message, MOCK_NEW_CONTACTS_OK, 0, NULL);
    bsal_actor_send(actor, source, message);

    /* say hello to remote actor too !
     */
    bsal_message_init(message, BUDDY_HELLO, 0, NULL);
    bsal_actor_send(actor, mock1->remote_actor, message);
}

void mock_die(struct bsal_actor *actor, struct bsal_message *message)
{
    int name;
    int source;

    name = bsal_actor_name(actor);
    source = bsal_message_source(message);

    printf("mock_die actor %i dies (MOCK_DIE from %i)\n", name, source);

    bsal_message_init(message, BSAL_ACTOR_STOP, 0, NULL);
    bsal_actor_send(actor, name, message);
}

void mock_start(struct bsal_actor *actor, struct bsal_message *message)
{
    int name;

    name = bsal_actor_name(actor);

    printf("actor %i spawn a child\n", name);

    /* spawn 1 child
     */
    mock_spawn_children(actor);
}

void mock_share(struct bsal_actor *actor, struct bsal_message *message)
{
    int name;
    struct mock *mock1;
    struct bsal_message message2;
    int next;

    mock1 = (struct mock *)bsal_actor_concrete_actor(actor);
    name = bsal_actor_name(actor);

    /* get the next mock actor
     */
    next = (name + 1) % bsal_vector_size(&mock1->spawners);

    bsal_message_init(&message2, MOCK_NEW_CONTACTS, 3 * sizeof(int),
                    (char *)mock1->children);
    bsal_actor_send(actor, next, &message2);
    bsal_message_destroy(&message2);
}

void mock_spawn_children(struct bsal_actor *actor)
{
    int total;
    struct mock *mock;
    int i;
    int name;
    struct bsal_message message;

    bsal_message_init(&message, BUDDY_BOOT, 0, NULL);

    total = 1;
    mock = (struct mock*)bsal_actor_concrete_actor(actor);

    for (i = 0; i <total; i++) {

        name = bsal_actor_spawn(actor, BUDDY_SCRIPT);

        bsal_actor_send(actor, name, &message);

        mock->children[i] = name;
    }

    bsal_message_destroy(&message);
}
