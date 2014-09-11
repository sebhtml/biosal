
#include "mock.h"
#include "buddy.h"

#include <core/system/debugger.h>

#include <stdio.h>

struct thorium_script mock_script = {
    .identifier = SCRIPT_MOCK,
    .init = mock_init,
    .destroy = mock_destroy,
    .receive = mock_receive,
    .size = sizeof(struct mock),
    .name = "mock"
};

void mock_init(struct thorium_actor *actor)
{
    struct mock *mock1;

    mock1 = (struct mock *)thorium_actor_concrete_actor(actor);
    mock1->value = 42;
    mock1->children[0] = -1;
    mock1->children[1] = -1;
    mock1->children[2] = -1;
    mock1->notified = 0;

    bsal_vector_init(&mock1->spawners, sizeof(int));
    thorium_actor_add_script(actor, SCRIPT_BUDDY, &buddy_script);
}

void mock_destroy(struct thorium_actor *actor)
{
    struct mock *mock1;

    mock1 = (struct mock *)thorium_actor_concrete_actor(actor);
    mock1->value = -1;

    bsal_vector_destroy(&mock1->spawners);
}

void mock_receive(struct thorium_actor *actor, struct thorium_message *message)
{
    int tag;
    int name;
    struct mock *mock1;
    char *buffer;
    int destination;
    struct thorium_message new_message;

    mock1 = (struct mock *)thorium_actor_concrete_actor(actor);

    tag = thorium_message_action(message);
    buffer = thorium_message_buffer(message);
    /*thorium_actor_print(actor);*/

    if (tag == ACTION_START) {

        bsal_vector_init(&mock1->spawners, 0);
        bsal_vector_unpack(&mock1->spawners, buffer);

        printf("ACTION_START spawners: %d\n",
                        (int)bsal_vector_size(&mock1->spawners));

        /*mock_init(actor);*/
        mock_start(actor, message);

    } else if (tag == ACTION_ASK_TO_STOP) {
        printf("MOCK_DIE\n");
        mock_die(actor, message);

    } else if (tag == ACTION_MOCK_NEW_CONTACTS) {
        printf("ACTION_MOCK_NEW_CONTACTS\n");
        mock_add_contacts(actor, message);

    } else if (tag == ACTION_MOCK_NEW_CONTACTS_REPLY) {

    } else if (tag == ACTION_BUDDY_HELLO_REPLY) {

        printf("ACTION_BUDDY_HELLO_OK\n");

        BSAL_DEBUGGER_ASSERT(bsal_vector_size(&mock1->spawners) > 0);

        destination = *(int *)bsal_vector_at(&mock1->spawners, 0);

        thorium_message_init(&new_message, ACTION_MOCK_NOTIFY, 0, NULL);
        thorium_actor_send(actor, destination, &new_message);
        thorium_message_destroy(&new_message);

    } else if (tag == ACTION_MOCK_NOTIFY) {

        mock1->notified++;

        printf("notifications %d/%d\n", mock1->notified,
                        (int)bsal_vector_size(&mock1->spawners));

        if (mock1->notified == bsal_vector_size(&mock1->spawners)) {
            thorium_message_init(message, ACTION_MOCK_PREPARE_DEATH, 0, NULL);

            /* the default binomial-tree algorithm can not
             * be used here because proxy actors may die
             * before they are needed.
             */
            thorium_actor_send_range(actor, &mock1->spawners, message);

            printf("Stopping all initial actors now\n");
        }

    } else if (tag == ACTION_MOCK_PREPARE_DEATH) {
        thorium_message_init(message, ACTION_ASK_TO_STOP, 0, NULL);

        name = thorium_actor_name(actor);
        thorium_actor_send(actor, name, message);

        thorium_message_init(message, ACTION_ASK_TO_STOP, 0, NULL);
        thorium_actor_send(actor, mock1->children[0], message);

    } else if (tag == ACTION_BUDDY_BOOT_REPLY) {

        mock_share(actor, message);
    }
}

void mock_add_contacts(struct thorium_actor *actor, struct thorium_message *message)
{
    int source;
    char *buffer;
    struct mock *mock1;

    mock1 = (struct mock *)thorium_actor_concrete_actor(actor);
    source = thorium_message_source(message);
    buffer = thorium_message_buffer(message);
    mock1->remote_actor = ((int*)buffer)[0];

    printf("mock_receive remote friend is %i\n",
                        mock1->remote_actor);

    thorium_message_init(message, ACTION_MOCK_NEW_CONTACTS_REPLY, 0, NULL);
    thorium_actor_send(actor, source, message);

    /* say hello to remote actor too !
     */
    thorium_message_init(message, ACTION_BUDDY_HELLO, 0, NULL);
    thorium_actor_send(actor, mock1->remote_actor, message);
}

void mock_die(struct thorium_actor *actor, struct thorium_message *message)
{
    int name;
    int source;

    name = thorium_actor_name(actor);
    source = thorium_message_source(message);

    printf("mock_die actor %i dies (MOCK_DIE from %i)\n", name, source);

    thorium_message_init(message, ACTION_STOP, 0, NULL);
    thorium_actor_send(actor, name, message);
}

void mock_start(struct thorium_actor *actor, struct thorium_message *message)
{
    int name;

    name = thorium_actor_name(actor);

    printf("actor %i spawn a child\n", name);

    /* spawn 1 child
     */
    mock_spawn_children(actor);
}

void mock_share(struct thorium_actor *actor, struct thorium_message *message)
{
    int name;
    struct mock *mock1;
    struct thorium_message message2;
    int next;
    int index;

    mock1 = (struct mock *)thorium_actor_concrete_actor(actor);
    name = thorium_actor_name(actor);
    index = bsal_vector_index_of(&mock1->spawners, &name);

    /* get the next mock actor
     * Actually uses 0 because this example code is ill-designed.
     */
    next = (index + 0) % bsal_vector_size(&mock1->spawners);

    thorium_message_init(&message2, ACTION_MOCK_NEW_CONTACTS, 3 * sizeof(int),
                    (char *)mock1->children);
    thorium_actor_send(actor, *(int *)bsal_vector_at(&mock1->spawners, next), &message2);
    thorium_message_destroy(&message2);
}

void mock_spawn_children(struct thorium_actor *actor)
{
    int total;
    struct mock *mock;
    int i;
    int name;
    struct thorium_message message;

    thorium_message_init(&message, ACTION_BUDDY_BOOT, 0, NULL);

    total = 1;
    mock = (struct mock*)thorium_actor_concrete_actor(actor);

    for (i = 0; i <total; i++) {

        name = thorium_actor_spawn(actor, SCRIPT_BUDDY);

        thorium_actor_send(actor, name, &message);

        mock->children[i] = name;
    }

    thorium_message_destroy(&message);
}
