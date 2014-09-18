
#include "writer_process.h"

struct thorium_script bsal_writer_process_script = {
    .identifier = SCRIPT_WRITER_PROCESS,
    .name = "bsal_writer_process",
    .size = sizeof(struct bsal_writer_process),
    .init = bsal_writer_process_init,
    .destroy = bsal_writer_process_destroy,
    .receive = bsal_writer_process_receive
};

void bsal_writer_process_init(struct thorium_actor *self)
{
    struct bsal_writer_process *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    concrete_self->has_file = 0;

    printf("%s/%d is ready to do input/output operations\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self));
}

void bsal_writer_process_destroy(struct thorium_actor *self)
{
    struct bsal_writer_process *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    concrete_self->has_file = 0;
}

void bsal_writer_process_receive(struct thorium_actor *self, struct thorium_message *message)
{
    int action;
    int count;
    int source;
    char *buffer;
    char *file_name;
    struct bsal_writer_process *concrete_self;

    concrete_self = thorium_actor_concrete_actor(self);

    action = thorium_message_action(message);
    count = thorium_message_count(message);
    buffer = thorium_message_buffer(message);
    source = thorium_message_source(message);

    if (action == ACTION_OPEN) {

        if (concrete_self->has_file) {
            printf("actor error, already open, can not open\n");
            return;
        }

        file_name = buffer;

        bsal_buffered_file_writer_init(&concrete_self->writer, file_name);

        concrete_self->has_file = 1;

        thorium_actor_send_reply_empty(self, ACTION_OPEN_REPLY);

    } else if (action == ACTION_WRITE) {

        bsal_buffered_file_writer_write(&concrete_self->writer, buffer, count);

        thorium_actor_send_reply_empty(self, ACTION_WRITE_REPLY);

    } else if (action == ACTION_CLOSE) {

        if (!concrete_self->has_file) {
            printf("Error, can not close a file that is not open\n");
            return;
        }

        bsal_buffered_file_writer_destroy(&concrete_self->writer);

        concrete_self->has_file = 0;

        thorium_actor_send_reply_empty(self, ACTION_CLOSE_REPLY);

    } else if (action == ACTION_ASK_TO_STOP
                    && source == thorium_actor_supervisor(self)) {

        thorium_actor_send_to_self_empty(self, ACTION_STOP);

        thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP_REPLY);
    }
}

