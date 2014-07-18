
#include "scheduling_queue.h"

void bsal_scheduling_queue_init(struct bsal_scheduling_queue *queue)
{
    bsal_ring_queue_init(&queue->max_priority_queue, sizeof(struct bsal_actor *));
    bsal_ring_queue_init(&queue->high_priority_queue, sizeof(struct bsal_actor *));
    bsal_ring_queue_init(&queue->normal_priority_queue, sizeof(struct bsal_actor *));
    bsal_ring_queue_init(&queue->low_priority_queue, sizeof(struct bsal_actor *));

    queue->max_priority_dequeue_operations = 0;
    queue->high_priority_dequeue_operations = 0;
    queue->normal_priority_dequeue_operations = 0;
    queue->low_priority_dequeue_operations = 0;
}

void bsal_scheduling_queue_destroy(struct bsal_scheduling_queue *queue)
{
    bsal_ring_queue_destroy(&queue->max_priority_queue);
    bsal_ring_queue_destroy(&queue->high_priority_queue);
    bsal_ring_queue_destroy(&queue->normal_priority_queue);
    bsal_ring_queue_destroy(&queue->low_priority_queue);

    queue->max_priority_dequeue_operations = 0;
    queue->high_priority_dequeue_operations = 0;
    queue->normal_priority_dequeue_operations = 0;
    queue->low_priority_dequeue_operations = 0;
}

int bsal_scheduling_queue_enqueue(struct bsal_scheduling_queue *queue, struct bsal_actor *actor)
{
    int priority;

    priority = BSAL_SCHEDULING_QUEUE_PRIORITY_NORMAL;

    if (priority == BSAL_SCHEDULING_QUEUE_PRIORITY_NORMAL) {
        return bsal_ring_queue_enqueue(&queue->normal_priority_queue, &actor);

    } else if (priority == BSAL_SCHEDULING_QUEUE_PRIORITY_MAX) {
        return bsal_ring_queue_enqueue(&queue->max_priority_queue, &actor);

    } else if (priority == BSAL_SCHEDULING_QUEUE_PRIORITY_HIGH) {
        return bsal_ring_queue_enqueue(&queue->high_priority_queue, &actor);

    } else /* if (priority == BSAL_SCHEDULING_QUEUE_PRIORITY_LOW) */ {

        return bsal_ring_queue_enqueue(&queue->low_priority_queue, &actor);
    }

    return 0;
}

int bsal_scheduling_queue_dequeue(struct bsal_scheduling_queue *queue, struct bsal_actor **actor)
{
    int low_size;
    int normal_size;
    int high_size;
    int max_size;
    uint64_t allowed_normal_operations;
    uint64_t allowed_low_operations;

    max_size = bsal_ring_queue_size(&queue->max_priority_queue);

    /*
     * If the max priority queue has stuff
     * it wins right away, regardless of anything else.
     */
    if (max_size > 0) {
        ++queue->max_priority_dequeue_operations;
        return bsal_ring_queue_dequeue(&queue->max_priority_queue, actor);
    }

    /* Otherwise, the multiplier is used.
     */

    low_size = bsal_ring_queue_size(&queue->low_priority_queue);
    normal_size = bsal_ring_queue_size(&queue->normal_priority_queue);
    high_size = bsal_ring_queue_size(&queue->high_priority_queue);

    /*
     * If the high priority queue has stuff
     * and normal and low queues are empty, then
     * high queue wins right away.
     */
    if (high_size > 0 && low_size == 0 && normal_size == 0) {
        ++queue->high_priority_dequeue_operations;
        return bsal_ring_queue_dequeue(&queue->high_priority_queue, actor);
    }

    /*
     * Otherwise, verify if we are allowed to dequeue from the
     * high priority queue.
     */

    allowed_normal_operations = queue->high_priority_dequeue_operations / BSAL_SCHEDULING_QUEUE_RATIO;
    allowed_low_operations = allowed_normal_operations / BSAL_SCHEDULING_QUEUE_RATIO;

    if (high_size > 0
             && queue->normal_priority_dequeue_operations >= allowed_normal_operations
             && queue->low_priority_dequeue_operations >= allowed_low_operations) {

        ++queue->high_priority_dequeue_operations;
        return bsal_ring_queue_dequeue(&queue->high_priority_queue, actor);
    }

    /* At this point, it is know that:
     *
     * 1. The max priority queue is empty.
     * 2. The high priority queue is empty OR
     *     the normal dequeue operations are below the allowed number of normal dequeue operations
     *     (which means that the dequeuing must be done on the normal queue or the low queue at this
     *     point, if of course the low priority queue or the normal priority queue are not empty.
     *
     * Therefore, below this line, only the normal priority queue and the low priority queue
     * are tested.
     */

    /*
     * If normal and low queues are empty,
     * return 0 (nothing was dequeued).
     */

    if (normal_size == 0 && low_size == 0) {
        return 0;
    }

    /*
     * Otherwise, if the low priority queue is empty,
     * and the normal queue has stuff, dequeue from normal
     */

    if (normal_size > 0 && low_size == 0) {

        ++queue->normal_priority_dequeue_operations;
        return bsal_ring_queue_dequeue(&queue->normal_priority_queue, actor);
    }

    /* Otherwise, if the low priority queue has stuff, but the normal
     * is empty, dequeue from normal.
     */

    if (normal_size == 0 && low_size > 0) {

        ++queue->low_priority_dequeue_operations;
        return bsal_ring_queue_dequeue(&queue->low_priority_queue, actor);
    }

    /* At this point, the low priority queue and the normal priority queue
     * both have things inside them and the max priority queue
     * and the high priority queue are empty (or the high fair-share disallows from
     * dequeuing from it.
     *
     * The scheduling queue must select either the normal priority queue or the
     * low priority queue.
     *
     * To do so, the ratio BSAL_SCHEDULING_QUEUE_RATIO
     * is used.
     *
     * That is, 1 dequeue operation on the low priority queue is allowed for each
     * BSAL_SCHEDULING_QUEUE_RATIO dequeue operations on the
     * normal priority queue.
     */

    allowed_low_operations = queue->normal_priority_dequeue_operations / BSAL_SCHEDULING_QUEUE_RATIO;

    /*
     * Use the low priority queue if it has not exceeded the limit
     * allowed.
     */
    if (queue->low_priority_dequeue_operations < allowed_low_operations) {

        ++queue->low_priority_dequeue_operations;
        return bsal_ring_queue_dequeue(&queue->low_priority_queue, actor);
    }

    /* Otherwise, use the normal priority queue directly.
     */

    ++queue->normal_priority_dequeue_operations;
    return bsal_ring_queue_dequeue(&queue->normal_priority_queue, actor);

}

int bsal_scheduling_queue_size(struct bsal_scheduling_queue *queue)
{
    int size;

    size = 0;
    size += bsal_ring_queue_size(&queue->max_priority_queue);
    size += bsal_ring_queue_size(&queue->high_priority_queue);
    size += bsal_ring_queue_size(&queue->normal_priority_queue);
    size += bsal_ring_queue_size(&queue->low_priority_queue);

    return size;
}
