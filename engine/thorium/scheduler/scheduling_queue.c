
#include "scheduling_queue.h"

#include <engine/thorium/actor.h>

#include <core/system/debugger.h>

void thorium_scheduling_queue_init(struct thorium_scheduling_queue *queue)
{
    bsal_fast_queue_init(thorium_scheduling_queue_select_queue(queue, THORIUM_PRIORITY_MAX),
                    sizeof(struct thorium_actor *));
    bsal_fast_queue_init(thorium_scheduling_queue_select_queue(queue, THORIUM_PRIORITY_HIGH),
                    sizeof(struct thorium_actor *));
    bsal_fast_queue_init(thorium_scheduling_queue_select_queue(queue, THORIUM_PRIORITY_NORMAL),
                    sizeof(struct thorium_actor *));
    bsal_fast_queue_init(thorium_scheduling_queue_select_queue(queue, THORIUM_PRIORITY_LOW),
                    sizeof(struct thorium_actor *));

    thorium_scheduling_queue_reset_counter(queue, THORIUM_PRIORITY_MAX);
    thorium_scheduling_queue_reset_counter(queue, THORIUM_PRIORITY_HIGH);
    thorium_scheduling_queue_reset_counter(queue, THORIUM_PRIORITY_NORMAL);
    thorium_scheduling_queue_reset_counter(queue, THORIUM_PRIORITY_LOW);
}

void thorium_scheduling_queue_destroy(struct thorium_scheduling_queue *queue)
{
    bsal_fast_queue_destroy(thorium_scheduling_queue_select_queue(queue, THORIUM_PRIORITY_MAX));
    bsal_fast_queue_destroy(thorium_scheduling_queue_select_queue(queue, THORIUM_PRIORITY_HIGH));
    bsal_fast_queue_destroy(thorium_scheduling_queue_select_queue(queue, THORIUM_PRIORITY_NORMAL));
    bsal_fast_queue_destroy(thorium_scheduling_queue_select_queue(queue, THORIUM_PRIORITY_LOW));

    thorium_scheduling_queue_reset_counter(queue, THORIUM_PRIORITY_MAX);
    thorium_scheduling_queue_reset_counter(queue, THORIUM_PRIORITY_HIGH);
    thorium_scheduling_queue_reset_counter(queue, THORIUM_PRIORITY_NORMAL);
    thorium_scheduling_queue_reset_counter(queue, THORIUM_PRIORITY_LOW);
}

int thorium_scheduling_queue_enqueue(struct thorium_scheduling_queue *queue, struct thorium_actor *actor)
{
    int priority;
    struct bsal_fast_queue *selected_queue;

    BSAL_DEBUGGER_ASSERT(actor != NULL);

    priority = thorium_actor_get_priority(actor);

    selected_queue = thorium_scheduling_queue_select_queue(queue, priority);

    return bsal_fast_queue_enqueue(selected_queue, &actor);
}

int thorium_scheduling_queue_dequeue(struct thorium_scheduling_queue *queue, struct thorium_actor **actor)
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

    max_size = thorium_scheduling_queue_get_size_with_priority(queue, THORIUM_PRIORITY_MAX);

    /*
     * If the max priority queue has stuff
     * it wins right away, regardless of anything else.
     */
    if (max_size > 0) {
#if 0
        printf("Got THORIUM_PRIORITY_MAX !\n");
#endif
        return thorium_scheduling_queue_dequeue_with_priority(queue, THORIUM_PRIORITY_MAX, actor);
    }

    /* Otherwise, the multiplier is used.
     */

    low_size = thorium_scheduling_queue_get_size_with_priority(queue, THORIUM_PRIORITY_LOW);
    normal_size = thorium_scheduling_queue_get_size_with_priority(queue, THORIUM_PRIORITY_NORMAL);
    high_size = thorium_scheduling_queue_get_size_with_priority(queue, THORIUM_PRIORITY_HIGH);

    /*
     * If the high priority queue has stuff
     * and normal and low queues are empty, then
     * high queue wins right away.
     */
    if (high_size > 0 && low_size == 0 && normal_size == 0) {
        return thorium_scheduling_queue_dequeue_with_priority(queue, THORIUM_PRIORITY_HIGH, actor);
    }

    /*
     * Otherwise, verify if we are allowed to dequeue from the
     * high priority queue.
     */

    high_priority_operations = thorium_scheduling_queue_get_counter(queue, THORIUM_PRIORITY_HIGH);
    normal_priority_operations = thorium_scheduling_queue_get_counter(queue, THORIUM_PRIORITY_NORMAL);
    low_priority_operations = thorium_scheduling_queue_get_counter(queue, THORIUM_PRIORITY_LOW);

    allowed_normal_operations = high_priority_operations / THORIUM_SCHEDULING_QUEUE_RATIO;
    allowed_low_operations = allowed_normal_operations / THORIUM_SCHEDULING_QUEUE_RATIO;

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

        return thorium_scheduling_queue_dequeue_with_priority(queue, THORIUM_PRIORITY_HIGH, actor);
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

        return thorium_scheduling_queue_dequeue_with_priority(queue, THORIUM_PRIORITY_NORMAL, actor);
    }

    /* Otherwise, if the low priority queue has stuff, but the normal
     * is empty, dequeue from normal.
     */

    if (normal_size == 0 && low_size > 0) {

        return thorium_scheduling_queue_dequeue_with_priority(queue, THORIUM_PRIORITY_LOW, actor);
    }

    /* At this point, the low priority queue and the normal priority queue
     * both have things inside them and the max priority queue
     * and the high priority queue are empty (or the high fair-share disallows from
     * dequeuing from it.
     *
     * The scheduling queue must select either the normal priority queue or the
     * low priority queue.
     *
     * To do so, the ratio THORIUM_SCHEDULING_QUEUE_RATIO
     * is used.
     *
     * That is, 1 dequeue operation on the low priority queue is allowed for each
     * THORIUM_SCHEDULING_QUEUE_RATIO dequeue operations on the
     * normal priority queue.
     */

    allowed_low_operations = normal_priority_operations / THORIUM_SCHEDULING_QUEUE_RATIO;

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

        return thorium_scheduling_queue_dequeue_with_priority(queue, THORIUM_PRIORITY_LOW, actor);
    }

    /* Otherwise, use the normal priority queue directly.
     */

    return thorium_scheduling_queue_dequeue_with_priority(queue, THORIUM_PRIORITY_NORMAL, actor);
}

int thorium_scheduling_queue_size(struct thorium_scheduling_queue *queue)
{
    int size;

    size = 0;

    size += thorium_scheduling_queue_get_size_with_priority(queue, THORIUM_PRIORITY_LOW);
    size += thorium_scheduling_queue_get_size_with_priority(queue, THORIUM_PRIORITY_NORMAL);
    size += thorium_scheduling_queue_get_size_with_priority(queue, THORIUM_PRIORITY_HIGH);
    size += thorium_scheduling_queue_get_size_with_priority(queue, THORIUM_PRIORITY_MAX);

    return size;
}

int thorium_scheduling_queue_get_size_with_priority(struct thorium_scheduling_queue *queue, int priority)
{
    struct bsal_fast_queue *selected_queue;

    selected_queue = thorium_scheduling_queue_select_queue(queue, priority);

    return bsal_fast_queue_size(selected_queue);
}

int thorium_scheduling_queue_dequeue_with_priority(struct thorium_scheduling_queue *queue, int priority,
                struct thorium_actor **actor)
{
    int value;
    struct bsal_fast_queue *selected_queue;
    uint64_t *selected_counter;

    selected_queue = thorium_scheduling_queue_select_queue(queue, priority);

    selected_counter = thorium_scheduling_queue_select_counter(queue, priority);
    value = bsal_fast_queue_dequeue(selected_queue, actor);

    if (value) {
        (*selected_counter)++;
    }

    return value;
}

struct bsal_fast_queue *thorium_scheduling_queue_select_queue(struct thorium_scheduling_queue *queue, int priority)
{
    struct bsal_fast_queue *selection;

    selection = NULL;

    if (priority == THORIUM_PRIORITY_MAX) {
        selection = &queue->max_priority_queue;

    } else if (priority == THORIUM_PRIORITY_HIGH) {
        selection = &queue->high_priority_queue;

    } else if (priority == THORIUM_PRIORITY_NORMAL) {
        selection = &queue->normal_priority_queue;

    } else if (priority == THORIUM_PRIORITY_LOW) {
        selection = &queue->low_priority_queue;
    }

    BSAL_DEBUGGER_ASSERT(selection != NULL);

    return selection;
}

uint64_t *thorium_scheduling_queue_select_counter(struct thorium_scheduling_queue *queue, int priority)
{
    uint64_t *selection;

    selection = NULL;

    if (priority == THORIUM_PRIORITY_MAX) {
        selection = &queue->max_priority_dequeue_operations;

    } else if (priority == THORIUM_PRIORITY_HIGH) {
        selection = &queue->high_priority_dequeue_operations;

    } else if (priority == THORIUM_PRIORITY_NORMAL) {
        selection = &queue->normal_priority_dequeue_operations;

    } else if (priority == THORIUM_PRIORITY_LOW) {
        selection = &queue->low_priority_dequeue_operations;
    }

    BSAL_DEBUGGER_ASSERT(selection != NULL);

    return selection;
}

uint64_t thorium_scheduling_queue_get_counter(struct thorium_scheduling_queue *queue, int priority)
{
    uint64_t *counter;

    counter = thorium_scheduling_queue_select_counter(queue, priority);

    return *counter;
}

void thorium_scheduling_queue_reset_counter(struct thorium_scheduling_queue *queue, int priority)
{
    uint64_t *counter;

    counter = thorium_scheduling_queue_select_counter(queue, priority);

    *counter = 0;
}

void thorium_scheduling_queue_print(struct thorium_scheduling_queue *queue, int node, int worker)
{
    printf("node/%d worker/%d SchedulingQueue Levels: %d\n",
                    node, worker, 4);

    thorium_scheduling_queue_print_with_priority(queue, THORIUM_PRIORITY_MAX, "THORIUM_PRIORITY_MAX", node, worker);
    thorium_scheduling_queue_print_with_priority(queue, THORIUM_PRIORITY_HIGH, "THORIUM_PRIORITY_HIGH", node, worker);
    thorium_scheduling_queue_print_with_priority(queue, THORIUM_PRIORITY_NORMAL, "THORIUM_PRIORITY_NORMAL", node, worker);
    thorium_scheduling_queue_print_with_priority(queue, THORIUM_PRIORITY_LOW, "THORIUM_PRIORITY_LOW", node, worker);

    printf("node/%d worker/%d SchedulingQueue... completed report !\n",
                    node, worker);
}

void thorium_scheduling_queue_print_with_priority(struct thorium_scheduling_queue *queue, int priority, const char *name,
                int node, int worker)
{
    struct bsal_fast_queue *selection;
    struct thorium_actor *actor;
    int size;
    int i;

    selection = thorium_scheduling_queue_select_queue(queue, priority);
    size = bsal_fast_queue_size(selection);

    printf("node/%d worker/%d scheduling_queue: Priority Queue %d (%s), actors: %d\n",
                    node, worker,
                    priority, name, size);

    i = 0;

    while (i < size) {
        bsal_fast_queue_dequeue(selection, &actor);
        bsal_fast_queue_enqueue(selection, &actor);

        printf("node/%d worker/%d [%i] actor %s/%d (%d messages)\n",
                        node, worker,
                        i,
                        thorium_actor_script_name(actor),
                        thorium_actor_name(actor),
                        thorium_actor_get_mailbox_size(actor));

        ++i;
    }
}
