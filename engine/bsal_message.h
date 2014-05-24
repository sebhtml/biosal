
#ifndef _BSAL_MESSAGE_H
#define _BSAL_MESSAGE_H

struct bsal_message {
    int source_actor;
    int destination_actor;
    int source_rank;
    int destination_rank;
    int routing_source;
    int routing_destination;
    int tag;
    char *buffer;
    int bytes;
};
typedef struct bsal_message bsal_message_t;

void bsal_message_construct(struct bsal_message *message, int tag,
                int source, int destination, int bytes, char *buffer);
void bsal_message_destruct(struct bsal_message *message);

int bsal_message_source(struct bsal_message *message);
int bsal_message_destination(struct bsal_message *message);
int bsal_message_source_rank(struct bsal_message *message);
int bsal_message_destination_rank(struct bsal_message *message);
int bsal_message_tag(struct bsal_message *message);

void bsal_message_set_source(struct bsal_message *message, int source);
void bsal_message_set_destination(struct bsal_message *message, int destination);
void bsal_message_print(struct bsal_message *message);

#endif
