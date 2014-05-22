
#ifndef _SAL_MESSAGE_H
#define _SAL_MESSAGE_H

struct bsal_message {
    int source_actor;
    int destination_actor;
    int source_rank;
    int destination_rank;
    int routing_source;
    int routing_destination;
};
typedef struct bsal_message bsal_message_t;

int bsal_message_get_source_actor(struct bsal_message *message);
int bsal_message_get_destination_actor(struct bsal_message *message);
int bsal_message_get_source_rank(struct bsal_message *message);
int bsal_message_get_destination_rank(struct bsal_message *message);

#endif
