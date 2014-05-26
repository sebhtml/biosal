
#ifndef _BSAL_THREAD_H
#define _BSAL_THREAD_H

#include "fifo.h"

#include <pthread.h>

struct bsal_work;
struct bsal_node;
struct bsal_message;

#define BSAL_THREAD_USE_MUTEX

struct bsal_thread {
    struct bsal_node *node;
    pthread_t thread;

    int name;
    volatile int dead;

    struct bsal_fifo works;
    struct bsal_fifo messages;

#ifdef BSAL_THREAD_USE_MUTEX
    pthread_mutex_t work_mutex;
    pthread_mutex_t message_mutex;
#endif
};

void bsal_thread_init(struct bsal_thread *thread, int name, struct bsal_node *node);
void bsal_thread_destroy(struct bsal_thread *thread);

struct bsal_fifo *bsal_thread_works(struct bsal_thread *thread);
struct bsal_fifo *bsal_thread_messages(struct bsal_thread *thread);

void bsal_thread_start(struct bsal_thread *thread);
void bsal_thread_stop(struct bsal_thread *thread);
pthread_t *bsal_thread_thread(struct bsal_thread *thread);

void bsal_thread_run(struct bsal_thread *thread);
void bsal_thread_work(struct bsal_thread *thread, struct bsal_work *work);
struct bsal_node *bsal_thread_node(struct bsal_thread *thread);

/*
void bsal_thread_receive(struct bsal_thread *thread, struct bsal_message *message);
*/
void bsal_thread_send(struct bsal_thread *thread, struct bsal_message *message);

void *bsal_thread_main(void *pointer);
int bsal_thread_name(struct bsal_thread *thread);
void bsal_thread_display(struct bsal_thread *thread);

void bsal_thread_push_work(struct bsal_thread *thread, struct bsal_work *work);
int bsal_thread_pull_work(struct bsal_thread *thread, struct bsal_work *work);

void bsal_thread_push_message(struct bsal_thread *thread, struct bsal_message *message);
int bsal_thread_pull_message(struct bsal_thread *thread, struct bsal_message *message);

#endif
