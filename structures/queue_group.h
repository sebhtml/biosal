
#ifndef _BSAL_FIFO_ARRAY_H
#define _BSAL_FIFO_ARRAY_H

/*#define BSAL_FIFO_ARRAY_VOLATILE */

/*
 * sizeof(char) is always 1 byte in C 1999
 * \see http://stackoverflow.com/questions/2215445/are-there-machines-where-sizeofchar-1
 * \see https://sites.google.com/site/kjellhedstrom2/threadsafecircularqueue
 * \see http://www.codeproject.com/Articles/43510/Lock-Free-Single-Producer-Single-Consumer-Circular
 */
struct bsal_queue_group {
    void *array;

    struct bsal_queue_group *previous;
    struct bsal_queue_group *next;

    int units;
    int bytes_per_unit;

#ifdef BSAL_FIFO_ARRAY_VOLATILE
    volatile int consumer_head;
    volatile int producer_tail;
#else
    int consumer_head;
    int producer_tail;
#endif
};

void bsal_queue_group_init(struct bsal_queue_group *queue, int units, int bytes_per_unit);
void bsal_queue_group_destroy(struct bsal_queue_group *queue);
void bsal_queue_group_reset(struct bsal_queue_group *queue);
int bsal_queue_group_push(struct bsal_queue_group *queue, void *item);
int bsal_queue_group_pop(struct bsal_queue_group *queue, void *item);

void *bsal_queue_group_unit(struct bsal_queue_group *queue, int position);
int bsal_queue_group_bytes(struct bsal_queue_group *queue);
int bsal_queue_group_empty(struct bsal_queue_group *queue);
int bsal_queue_group_full(struct bsal_queue_group *queue);

void bsal_queue_group_set_next(struct bsal_queue_group *queue, struct bsal_queue_group *item);
void bsal_queue_group_set_previous(struct bsal_queue_group *queue, struct bsal_queue_group *item);
struct bsal_queue_group *bsal_queue_group_next(struct bsal_queue_group *queue);
struct bsal_queue_group *bsal_queue_group_previous(struct bsal_queue_group *queue);

#endif
