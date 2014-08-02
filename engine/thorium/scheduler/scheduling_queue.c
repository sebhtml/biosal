
#include "scheduling_queue.h"

#include <engine/thorium/actor.h>

#include <core/system/debugger.h>

void bsal_scheduling_queue_init(struct bsal_scheduling_queue *queue)
{
    bsal_ring_queue_init(bsal_scheduling_queue_select_queue(queue, BSAL_PRIORITY_MAX),
                    sizeof(struct bsal_actor *));
    bsal_ring_queue_init(bsal_scheduling_queue_select_queue(queue, BSAL_PRIORITY_HIGH),
                    sizeof(struct bsal_actor *));
    bsal_ring_queue_init(bsal_scheduling_queue_select_queue(queue, BSAL_PRIORITY_NORMAL),
                    sizeof(struct bsal_actor *));
    bsal_ring_queue_init(bsal_scheduling_queue_select_queue(queue, BSAL_PRIORITY_LOW),
                    sizeof(struct bsal_actor *));

    bsal_scheduling_queue_reset_counter(queue, BSAL_PRIORITY_MAX);
    bsal_scheduling_queue_reset_counter(queue, BSAL_PRIORITY_HIGH);
    bsal_scheduling_queue_reset_counter(queue, BSAL_PRIORITY_NORMAL);
    bsal_scheduling_queue_reset_counter(queue, BSAL_PRIORITY_LOW);
}

void bsal_scheduling_queue_destroy(struct bsal_scheduling_queue *queue)
{
    bsal_ring_queue_destroy(bsal_scheduling_queue_select_queue(queue, BSAL_PRIORITY_MAX));
    bsal_ring_queue_destroy(bsal_scheduling_queue_select_queue(queue, BSAL_PRIORITY_HIGH));
    bsal_ring_queue_destroy(bsal_scheduling_queue_select_queue(queue, BSAL_PRIORITY_NORMAL));
    bsal_ring_queue_destroy(bsal_scheduling_queue_select_queue(queue, BSAL_PRIORITY_LOW));

    bsal_scheduling_queue_reset_counter(queue, BSAL_PRIORITY_MAX);
    bsal_scheduling_queue_reset_counter(queue, BSAL_PRIORITY_HIGH);
    bsal_scheduling_queue_reset_counter(queue, BSAL_PRIORITY_NORMAL);
    bsal_scheduling_queue_reset_counter(queue, BSAL_PRIORITY_LOW);
}

int bsal_scheduling_queue_enqueue(struct bsal_scheduling_queue *queue, struct bsal_actor *actor)
{
    int priority;
    struct bsal_ring_queue *selected_queue;

    BSAL_DEBUGGER_ASSERT(actor != NULL);

    priority = bsal_actor_get_priority(actor);

    selected_queue = bsal_scheduling_queue_select_queue(queue, priority);

    return bsal_ring_queue_enqueue(selected_queue, &actor);
}

int bsal_scheduling_queue_dequeue(struct bsal_scheduling_queue *queue, struct bsal_actor **actor)
{
    int low_size;
    int normal_size;
    int high_size;
    int max_size;
    int normal_limit_reached;
    int low_limit_reached;

    uint64_t high_priority_operations;
    uint64_t normal_priority_operations;
    uint64_t low_priority_operations;

    uint64_t allowed_normal_operations;
    uint64_t allowed_low_operations;

    max_size = bsal_scheduling_queue_get_size_with_priority(queue, BSAL_PRIORITY_MAX);

    /*
     * If the max priority queue has stuff
     * it wins right away, regardless of anything else.
     */
    if (max_size > 0) {
        return bsal_scheduling_queue_dequeue_with_priority(queue, BSAL_PRIORITY_MAX, actor);
    }

    /* Otherwise, the multiplier is used.
     */

    low_size = bsal_scheduling_queue_get_size_with_priority(queue, BSAL_PRIORITY_LOW);
    normal_size = bsal_scheduling_queue_get_size_with_priority(queue, BSAL_PRIORITY_NORMAL);
    high_size = bsal_scheduling_queue_get_size_with_priority(queue, BSAL_PRIORITY_HIGH);

    /*
     * If the high priority queue has stuff
     * and normal and low queues are empty, then
     * high queue wins right away.
     */
    if (high_size > 0 && low_size == 0 && normal_size == 0) {
        return bsal_scheduling_queue_dequeue_with_priority(queue, BSAL_PRIORITY_HIGH, actor);
    }

    /*
     * Otherwise, verify if we are allowed to dequeue from the
     * high priority queue.
     */

    high_priority_operations = bsal_scheduling_queue_get_counter(queue, BSAL_PRIORITY_HIGH);
    normal_priority_operations = bsal_scheduling_queue_get_counter(queue, BSAL_PRIORITY_NORMAL);
    low_priority_operations = bsal_scheduling_queue_get_counter(queue, BSAL_PRIORITY_LOW);

    allowed_normal_operations = high_priority_operations / BSAL_SCHEDULING_QUEUE_RATIO;
    allowed_low_operations = allowed_normal_operations / BSAL_SCHEDULING_QUEUE_RATIO;

    normal_limit_reached = 0;

    if (normal_priority_operations >= allowed_normal_operations) {
        normal_limit_reached = 1;
    }

    low_limit_reached = 0;

    if (low_priority_operations >= allowed_low_operations) {
        low_limit_reached = 1;
    }

    if (high_size > 0
             && normal_limit_reached
             && low_limit_reached) {

        return bsal_scheduling_queue_dequeue_with_priority(queue, BSAL_PRIORITY_HIGH, actor);
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

        return bsal_scheduling_queue_dequeue_with_priority(queue, BSAL_PRIORITY_NORMAL, actor);
    }

    /* Otherwise, if the low priority queue has stuff, but the normal
     * is empty, dequeue from normal.
     */

    if (normal_size == 0 && low_size > 0) {

        return bsal_scheduling_queue_dequeue_with_priority(queue, BSAL_PRIORITY_LOW, actor);
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

    allowed_low_operations = normal_priority_operations / BSAL_SCHEDULING_QUEUE_RATIO;

    low_limit_reached = 0;

    if (low_priority_operations >= allowed_low_operations) {
        low_limit_reached = 1;
    }

    /*
     * Use the low priority queue if it has not exceeded the limit
     * allowed.
     *
     * @low_limit_reached is either in comparison with the high priority or with the
     * low priority.
     *
     */
    if (!low_limit_reached) {

        return bsal_scheduling_queue_dequeue_with_priority(queue, BSAL_PRIORITY_LOW, actor);
    }

    /* Otherwise, use the normal priority queue directly.
     */

    return bsal_scheduling_queue_dequeue_with_priority(queue, BSAL_PRIORITY_NORMAL, actor);
}

int bsal_scheduling_queue_size(struct bsal_scheduling_queue *queue)
{
    int size;

    size = 0;

    size += bsal_scheduling_queue_get_size_with_priority(queue, BSAL_PRIORITY_LOW);
    size += bsal_scheduling_queue_get_size_with_priority(queue, BSAL_PRIORITY_NORMAL);
    size += bsal_scheduling_queue_get_size_with_priority(queue, BSAL_PRIORITY_HIGH);
    size += bsal_scheduling_queue_get_size_with_priority(queue, BSAL_PRIORITY_MAX);

    return size;
}

int bsal_scheduling_queue_get_size_with_priority(struct bsal_scheduling_queue *queue, int priority)
{
    struct bsal_ring_queue *selected_queue;

    selected_queue = bsal_scheduling_queue_select_queue(queue, priority);

    return bsal_ring_queue_size(selected_queue);
}

int bsal_scheduling_queue_dequeue_with_priority(struct bsal_scheduling_queue *queue, int priority,
                struct bsal_actor **actor)
{
    int value;
    struct bsal_ring_queue *selected_queue;
    uint64_t *selected_counter;

    selected_queue = bsal_scheduling_queue_select_queue(queue, priority);

    selected_counter = bsal_scheduling_queue_select_counter(queue, priority);
    value = bsal_ring_queue_dequeue(selected_queue, actor);

    if (value) {
        (*selected_counter)++;
    }

    return value;
}

struct bsal_ring_queue *bsal_scheduling_queue_select_queue(struct bsal_scheduling_queue *queue, int priority)
{
    struct bsal_ring_queue *selection;

    selection = NULL;

    if (priority == BSAL_PRIORITY_MAX) {
        selection = &queue->max_priority_queue;

    } else if (priority == BSAL_PRIORITY_HIGH) {
        selection = &queue->high_priority_queue;

    } else if (priority == BSAL_PRIORITY_NORMAL) {
        selection = &queue->normal_priority_queue;

    } else if (priority == BSAL_PRIORITY_LOW) {
        selection = &queue->low_priority_queue;
    }

    BSAL_DEBUGGER_ASSERT(selection != NULL);

    return selection;
}

uint64_t *bsal_scheduling_queue_select_counter(struct bsal_scheduling_queue *queue, int priority)
{
    uint64_t *selection;

    selection = NULL;

    if (priority == BSAL_PRIORITY_MAX) {
        selection = &queue->max_priority_dequeue_operations;

    } else if (priority == BSAL_PRIORITY_HIGH) {
        selection = &queue->high_priority_dequeue_operations;

    } else if (priority == BSAL_PRIORITY_NORMAL) {
        selection = &queue->normal_priority_dequeue_operations;

    } else if (priority == BSAL_PRIORITY_LOW) {
        selection = &queue->low_priority_dequeue_operations;
    }

    BSAL_DEBUGGER_ASSERT(selection != NULL);

    return selection;
}

uint64_t bsal_scheduling_queue_get_counter(struct bsal_scheduling_queue *queue, int priority)
{
    uint64_t *counter;

    counter = bsal_scheduling_queue_select_counter(queue, priority);

    return *counter;
}

void bsal_scheduling_queue_reset_counter(struct bsal_scheduling_queue *queue, int priority)
{
    uint64_t *counter;

    counter = bsal_scheduling_queue_select_counter(queue, priority);

    *counter = 0;
}

void bsal_scheduling_queue_print(struct bsal_scheduling_queue *queue, int node, int worker)
{
    printf("node/%d worker/%d SchedulingQueue Levels: %d\n",
                    node, worker, 4);

    bsal_scheduling_queue_print_with_priority(queue, BSAL_PRIORITY_MAX, "BSAL_PRIORITY_MAX", node, worker);
    bsal_scheduling_queue_print_with_priority(queue, BSAL_PRIORITY_HIGH, "BSAL_PRIORITY_HIGH", node, worker);
    bsal_scheduling_queue_print_with_priority(queue, BSAL_PRIORITY_NORMAL, "BSAL_PRIORITY_NORMAL", node, worker);
    bsal_scheduling_queue_print_with_priority(queue, BSAL_PRIORITY_LOW, "BSAL_PRIORITY_LOW", node, worker);

    printf("node/%d worker/%d SchedulingQueue... completed report !\n",
                    node, worker);
}

void bsal_scheduling_queue_print_with_priority(struct bsal_scheduling_queue *queue, int priority, const char *name,
                int node, int worker)
{
    struct bsal_ring_queue *selection;
    struct bsal_actor *actor;
    int size;
    int i;

    selection = bsal_scheduling_queue_select_queue(queue, priority);
    size = bsal_ring_queue_size(selection);

    printf("node/%d worker/%d scheduling_queue: Priority Queue %d (%s), actors: %d\n",
                    node, worker,
                    priority, name, size);

    i = 0;

    while (i < size) {
        bsal_ring_queue_dequeue(selection, &actor);
        bsal_ring_queue_enqueue(selection, &actor);

        printf("node/%d worker/%d [%i] actor %s/%d (%d messages)\n",
                        node, worker,
                        i,
                        bsal_actor_script_name(actor),
                        bsal_actor_name(actor),
                        bsal_actor_get_mailbox_size(actor));

        ++i;
    }
}
