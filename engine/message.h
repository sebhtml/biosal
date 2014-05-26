
#ifndef _BSAL_MESSAGE_H
#define _BSAL_MESSAGE_H

struct bsal_message {
    char *buffer;
    int count;
    int tag;

    int source_actor;
    int destination_actor;

    int source_rank;
    int destination_rank;

    int routing_source;
    int routing_destination;
};

typedef struct bsal_message bsal_message_t;

void bsal_message_init(struct bsal_message *message, int tag,
                int source, int destination, int count, char *buffer);
void bsal_message_destroy(struct bsal_message *message);

int bsal_message_source(struct bsal_message *message);
int bsal_message_destination(struct bsal_message *message);
int bsal_message_source_rank(struct bsal_message *message);
int bsal_message_destination_rank(struct bsal_message *message);
int bsal_message_tag(struct bsal_message *message);
void bsal_message_set_tag(struct bsal_message *message, int tag);

void bsal_message_set_source(struct bsal_message *message, int source);
void bsal_message_set_destination(struct bsal_message *message, int destination);
void bsal_message_print(struct bsal_message *message);
char *bsal_message_buffer(struct bsal_message *message);
void bsal_message_set_buffer(struct bsal_message *message, char *buffer);
int bsal_message_count(struct bsal_message *message);

void bsal_message_set_source_rank(struct bsal_message *message, int source);
void bsal_message_set_destination_rank(struct bsal_message *message, int destination);

int bsal_message_metadata_size(struct bsal_message *message);
void bsal_message_read_metadata(struct bsal_message *message);
void bsal_message_write_metadata(struct bsal_message *message);

#endif
