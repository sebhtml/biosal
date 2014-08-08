
#ifndef BSAL_WORKER_BUFFER_H
#define BSAL_WORKER_BUFFER_H

/*
 * This is a worker buffer.
 * if worker is lower than 0, then this means that this is
 * a Thorium core node buffer.
 */
struct bsal_worker_buffer {
    int worker;
    void *buffer;
};

void bsal_worker_buffer_init(struct bsal_worker_buffer *self, int worker, void *buffer);
void bsal_worker_buffer_destroy(struct bsal_worker_buffer *self);

int bsal_worker_buffer_get_worker(struct bsal_worker_buffer *self);
void *bsal_worker_buffer_get_buffer(struct bsal_worker_buffer *self);

#endif
