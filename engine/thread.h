
#ifndef _BSAL_THREAD_H
#define _BSAL_THREAD_H

#include "fifo.h"

#include <pthread.h>

struct bsal_work;
struct bsal_node;
struct bsal_message;

struct bsal_thread {
    struct bsal_node *node;
    pthread_t thread;

    struct bsal_fifo works;
    struct bsal_fifo messages;
};

void bsal_thread_init(struct bsal_thread *thread, struct bsal_node *node);
void bsal_thread_destroy(struct bsal_thread *thread);
struct bsal_fifo *bsal_thread_works(struct bsal_thread *thread);
struct bsal_fifo *bsal_thread_messages(struct bsal_thread *thread);

void bsal_thread_run(struct bsal_thread *thread);
void bsal_thread_work(struct bsal_thread *thread, struct bsal_work *work);
struct bsal_node *bsal_thread_node(struct bsal_thread *thread);

void bsal_thread_receive(struct bsal_thread *thread, struct bsal_message *message);
void bsal_thread_send(struct bsal_thread *thread, struct bsal_message *message);

#endif
