
#include "scheduler.h"

#include "worker_pool.h"
#include "migration.h"
#include "worker.h"
#include "actor.h"
#include "node.h"

#include <core/helpers/pair.h>
#include <core/helpers/vector_helper.h>
#include <core/helpers/statistics.h>

#include <core/structures/vector.h>
#include <core/structures/vector_iterator.h>
#include <core/structures/map.h>
#include <core/structures/map_iterator.h>

#include <stdio.h>

/* Parameters for the scheduler
 */
#define SCHEDULER_PRECISION 1000000

/*
 * Use the 10th percentile and the 90th percentile
 */
#define SCHEDULER_WINDOW 10

#define BSAL_SCHEDULER_WORK_SCHEDULING_WINDOW 8192

/*
 * Definitions for scheduling classes
 * See bsal_worker_pool_balance for classification requirements.
 */

#define BSAL_CLASS_STALLED_STRING "BSAL_CLASS_STALLED"
#define BSAL_CLASS_OPERATING_AT_FULL_CAPACITY_STRING "BSAL_CLASS_OPERATING_AT_FULL_CAPACITY"
#define BSAL_CLASS_NORMAL_STRING "BSAL_CLASS_NORMAL"
#define BSAL_CLASS_HUB_STRING "BSAL_CLASS_HUB"
#define BSAL_CLASS_BURDENED_STRING "BSAL_CLASS_BURDENED"



void bsal_scheduler_init(struct bsal_scheduler *scheduler, struct bsal_worker_pool *pool)
{
    scheduler->pool = pool;
    bsal_map_init(&scheduler->actor_affinities, sizeof(int), sizeof(int));
    bsal_map_init(&scheduler->last_actor_received_messages, sizeof(int), sizeof(int));
    
    scheduler->worker_for_work = 0;
}

void bsal_scheduler_destroy(struct bsal_scheduler *scheduler)
{
    scheduler->pool = NULL;
    bsal_map_destroy(&scheduler->actor_affinities);
    bsal_map_destroy(&scheduler->last_actor_received_messages);
}

void bsal_scheduler_balance(struct bsal_scheduler *scheduler)
{
    /*
     * The 95th percentile is useful:
     * \see http://en.wikipedia.org/wiki/Burstable_billing
     * \see http://www.init7.net/en/backbone/95-percent-rule
     */
    int load_percentile_50;

    int i;
    struct bsal_vector loads;
    struct bsal_vector loads_unsorted;
    struct bsal_vector burdened_workers;
    struct bsal_vector stalled_workers;
    struct bsal_worker *worker;

    /*struct bsal_set *set;*/
    struct bsal_pair pair;
    struct bsal_vector_iterator vector_iterator;
    int old_worker;
    int actor_name;
    int messages;
    int maximum;
    int with_maximum;
    struct bsal_map *set;
    struct bsal_map_iterator set_iterator;
    int stalled_index;
    int stalled_count;
    int new_worker_index;
    struct bsal_vector migrations;
    struct bsal_migration migration;
    struct bsal_migration *migration_to_do;
    struct bsal_actor *actor;
    int candidates;

    int load_value;
    int remaining_load;
    int projected_load;

    struct bsal_vector actors_to_migrate;
    int total;
    int with_messages;
    int stalled_percentile;
    int burdened_percentile;

    int old_total;
    int old_load;
    int new_load;
    int predicted_new_load;
    struct bsal_pair *pair_pointer;
    struct bsal_worker *new_worker;
    /*int new_total;*/
    int actor_load;

    int test_stalled_index;
    int tests;
    int found_match;

    /* Lock all workers first
     */
    for (i = 0; i < bsal_worker_pool_worker_count(scheduler->pool); i++) {
        worker = bsal_worker_pool_get_worker(scheduler->pool, i);

        bsal_worker_lock(worker);
    }


    bsal_vector_init(&migrations, sizeof(struct bsal_migration));
    printf("BALANCING\n");

    bsal_vector_init(&loads, sizeof(int));
    bsal_vector_init(&loads_unsorted, sizeof(int));
    bsal_vector_init(&burdened_workers, sizeof(struct bsal_pair));
    bsal_vector_init(&stalled_workers, sizeof(struct bsal_pair));

    bsal_vector_init(&actors_to_migrate, sizeof(struct bsal_pair));

    for (i = 0; i < bsal_worker_pool_worker_count(scheduler->pool); i++) {
        worker = bsal_worker_pool_get_worker(scheduler->pool, i);
        load_value = bsal_worker_get_scheduling_epoch_load(worker) * SCHEDULER_PRECISION;

#if 0
        printf("DEBUG LOAD %d %d\n", i, load_value);
#endif

        bsal_vector_push_back(&loads, &load_value);
        bsal_vector_push_back(&loads_unsorted, &load_value);
    }

    bsal_vector_helper_sort_int(&loads);

    stalled_percentile = bsal_statistics_get_percentile_int(&loads, SCHEDULER_WINDOW);
    /*load_percentile_25 = bsal_statistics_get_percentile_int(&loads, 25);*/
    load_percentile_50 = bsal_statistics_get_percentile_int(&loads, 50);
    /*load_percentile_75 = bsal_statistics_get_percentile_int(&loads, 75);*/
    burdened_percentile = bsal_statistics_get_percentile_int(&loads, 100 - SCHEDULER_WINDOW);

    printf("Percentiles for epoch loads: ");
    bsal_statistics_get_print_percentiles_int(&loads);

    for (i = 0; i < bsal_worker_pool_worker_count(scheduler->pool); i++) {
        worker = bsal_worker_pool_get_worker(scheduler->pool, i);
        load_value = bsal_vector_helper_at_as_int(&loads_unsorted, i);

        set = bsal_worker_get_actors(worker);

        if (stalled_percentile == burdened_percentile) {

            printf("scheduling_class:%s ",
                            BSAL_CLASS_NORMAL_STRING);

        } else if (load_value <= stalled_percentile) {

            printf("scheduling_class:%s ",
                            BSAL_CLASS_STALLED_STRING);
            bsal_pair_init(&pair, load_value, i);
            bsal_vector_push_back(&stalled_workers, &pair);

        } else if (load_value >= burdened_percentile) {

            printf("scheduling_class:%s ",
                            BSAL_CLASS_BURDENED_STRING);

            bsal_pair_init(&pair, load_value, i);
            bsal_vector_push_back(&burdened_workers, &pair);
        } else {
            printf("scheduling_class:%s ",
                            BSAL_CLASS_NORMAL_STRING);
        }

        bsal_worker_print_actors(worker, scheduler);

    }

    bsal_vector_helper_sort_int_reverse(&burdened_workers);
    bsal_vector_helper_sort_int(&stalled_workers);

    stalled_count = bsal_vector_size(&stalled_workers);
    printf("MIGRATIONS (stalled: %d, burdened: %d)\n", (int)bsal_vector_size(&stalled_workers),
                    (int)bsal_vector_size(&burdened_workers));

    stalled_index = 0;
    bsal_vector_iterator_init(&vector_iterator, &burdened_workers);

    while (stalled_count > 0
                    && bsal_vector_iterator_get_next_value(&vector_iterator, &pair)) {

        old_worker = bsal_pair_get_second(&pair);

        worker = bsal_worker_pool_get_worker(scheduler->pool, old_worker);
        set = bsal_worker_get_actors(worker);

        /*
        bsal_worker_print_actors(worker);
        printf("\n");
        */

        /*
         * Lock the worker and try to select actors for migration
         */
        bsal_map_iterator_init(&set_iterator, set);

        maximum = -1;
        with_maximum = 0;
        total = 0;
        with_messages = 0;

        while (bsal_map_iterator_get_next_key_and_value(&set_iterator, &actor_name, NULL)) {

            actor = bsal_node_get_actor_from_name(bsal_worker_pool_get_node(scheduler->pool), actor_name);
            messages = bsal_scheduler_get_actor_production(scheduler, actor);

            if (maximum == -1 || messages > maximum) {
                maximum = messages;
                with_maximum = 1;
            } else if (messages == maximum) {
                with_maximum++;
            }

            if (messages > 0) {
                ++with_messages;
            }

            total += messages;
        }

        bsal_map_iterator_destroy(&set_iterator);

        bsal_map_iterator_init(&set_iterator, set);

        --with_maximum;

        candidates = 0;
        load_value = bsal_worker_get_scheduling_epoch_load(worker) * SCHEDULER_PRECISION;

        remaining_load = load_value;

#if 0
        printf("maximum %d with_maximum %d\n", maximum, with_maximum);
#endif

        while (bsal_map_iterator_get_next_key_and_value(&set_iterator, &actor_name, NULL)) {

            actor = bsal_node_get_actor_from_name(bsal_worker_pool_get_node(scheduler->pool), actor_name);
            messages = bsal_scheduler_get_actor_production(scheduler, actor);

            /* Simulate the remaining load
             */
            projected_load = remaining_load;
            projected_load -= ((0.0 + messages) / total) * load_value;

            printf(" TESTING actor %d, production was %d, projected_load is %d (- %d * (1 - %d/%d)\n",
                            actor_name, messages, projected_load,
                            load_value, messages, total);

            /* An actor without any queued messages should not be migrated
             */
            if (messages > 0
                            && ((with_maximum > 0 && messages == maximum) || messages < maximum)
                /*
                 * Avoid removing too many actors because
                 * generating a stalled one is not desired
                 */
                    && (projected_load >= load_percentile_50

                /*
                 * The previous rule does not apply when there
                 * are 2 actors.
                 */
                   || with_messages == 2) ) {

                remaining_load = projected_load;

                candidates++;

                if (messages == maximum) {
                    --with_maximum;
                }


                bsal_pair_init(&pair, messages, actor_name);
                bsal_vector_push_back(&actors_to_migrate, &pair);

                printf("early CANDIDATE for migration: actor %d, worker %d\n",
                                actor_name, old_worker);
            }
        }
        bsal_map_iterator_destroy(&set_iterator);

    }

    bsal_vector_iterator_destroy(&vector_iterator);

    /* Sort the candidates
     */

    bsal_vector_helper_sort_int(&actors_to_migrate);

    /*
    printf("Percentiles for production: ");
    bsal_statistics_get_print_percentiles_int(&actors_to_migrate);
    */

    /* Sort them in reverse order.
     */
    bsal_vector_helper_sort_int_reverse(&actors_to_migrate);

    bsal_vector_iterator_init(&vector_iterator, &actors_to_migrate);

    while (bsal_vector_iterator_get_next_value(&vector_iterator, &pair)) {

        actor_name = bsal_pair_get_second(&pair);

        actor = bsal_node_get_actor_from_name(bsal_worker_pool_get_node(scheduler->pool), actor_name);

        if (actor == NULL) {
           continue;
        }

        messages = bsal_scheduler_get_actor_production(scheduler, actor);
        bsal_map_get_value(&scheduler->actor_affinities, &actor_name, &old_worker);

        worker = bsal_worker_pool_get_worker(scheduler->pool, old_worker);

        /* old_total can not be 0 because otherwise the would not
         * be burdened.
         */
        old_total = bsal_worker_get_production(worker, scheduler);
        with_messages = bsal_worker_get_producer_count(worker, scheduler);
        old_load = bsal_worker_get_scheduling_epoch_load(worker) * SCHEDULER_PRECISION;
        actor_load = ((0.0 + messages) / old_total) * old_load;

        /* Try to find a stalled worker that can take it.
         */

        test_stalled_index = stalled_index;
        tests = 0;
        predicted_new_load = 0;

        found_match = 0;
        while (tests < stalled_count) {

            bsal_vector_get_value(&stalled_workers, test_stalled_index, &pair);
            new_worker_index = bsal_pair_get_second(&pair);

            new_worker = bsal_worker_pool_get_worker(scheduler->pool, new_worker_index);
            new_load = bsal_worker_get_scheduling_epoch_load(new_worker) * SCHEDULER_PRECISION;
        /*new_total = bsal_worker_get_production(new_worker);*/

            predicted_new_load = new_load + actor_load;

            if (predicted_new_load > SCHEDULER_PRECISION /* && with_messages != 2 */) {
                printf("Scheduler: skipping actor %d, predicted load is %d >= 100\n",
                           actor_name, predicted_new_load);

                ++tests;
                ++test_stalled_index;

                if (test_stalled_index == stalled_count) {
                    test_stalled_index = 0;
                }
                continue;
            }

            /* Otherwise, this stalled worker is fine...
             */
            stalled_index = test_stalled_index;
            found_match = 1;

            break;
        }

        /* This actor can not be migrated to any stalled worker.
         */
        if (!found_match) {
            continue;
        }

        /* Otherwise, update the load of the stalled one and go forward with the change.
         */

        pair_pointer = (struct bsal_pair *)bsal_vector_at(&stalled_workers, stalled_index);

        bsal_pair_set_first(pair_pointer, predicted_new_load);

        ++stalled_index;

        if (stalled_index == stalled_count) {
            stalled_index = 0;
        }


#if 0
        new_worker = bsal_worker_pool_get_worker(pool, new_worker_index);
        printf(" CANDIDATE: actor %d old worker %d (%d - %d = %d) new worker %d (%d + %d = %d)\n",
                        actor_name,
                        old_worker, value, messages, 2new_score,
                        new_worker_index, new_worker_old_score, messages, new_worker_new_score);
#endif

        bsal_migration_init(&migration, actor_name, old_worker, new_worker_index);
        bsal_vector_push_back(&migrations, &migration);
        bsal_migration_destroy(&migration);

    }

    bsal_vector_iterator_destroy(&vector_iterator);

    bsal_vector_destroy(&stalled_workers);
    bsal_vector_destroy(&burdened_workers);
    bsal_vector_destroy(&loads);
    bsal_vector_destroy(&loads_unsorted);
    bsal_vector_destroy(&actors_to_migrate);

    /* Update the last values
     */
    for (i = 0; i < bsal_worker_pool_worker_count(scheduler->pool); i++) {

        worker = bsal_worker_pool_get_worker(scheduler->pool, i);

        set = bsal_worker_get_actors(worker);

        bsal_map_iterator_init(&set_iterator, set);

        while (bsal_map_iterator_get_next_key_and_value(&set_iterator, &actor_name, NULL)) {
            actor = bsal_node_get_actor_from_name(bsal_worker_pool_get_node(scheduler->pool), actor_name);
            bsal_scheduler_update_actor_production(scheduler, actor);
        }
        bsal_map_iterator_destroy(&set_iterator);

        bsal_worker_reset_scheduling_epoch(worker);
    }

    /* Actually do the migrations
     */
    bsal_vector_iterator_init(&vector_iterator, &migrations);

    while (bsal_vector_iterator_next(&vector_iterator, (void **)&migration_to_do)) {

        bsal_scheduler_migrate(scheduler, migration_to_do);
    }

    bsal_vector_iterator_destroy(&vector_iterator);

    bsal_vector_destroy(&migrations);

    /* Unlock all workers
     */
    for (i = 0; i < bsal_worker_pool_worker_count(scheduler->pool); i++) {
        worker = bsal_worker_pool_get_worker(scheduler->pool, i);

        bsal_worker_unlock(worker);
    }

}

void bsal_scheduler_migrate(struct bsal_scheduler *scheduler, struct bsal_migration *migration)
{
    struct bsal_worker *old_worker_object;
    struct bsal_worker *new_worker_object;
    int old_worker;
    int new_worker;
    int actor_name;
    struct bsal_actor *actor;

    old_worker = bsal_migration_get_old_worker(migration);
    new_worker = bsal_migration_get_new_worker(migration);
    actor_name = bsal_migration_get_actor(migration);
    actor = bsal_node_get_actor_from_name(bsal_worker_pool_get_node(scheduler->pool), actor_name);

    printf("MIGRATION node %d migrated actor %d from worker %d to worker %d\n",
                    bsal_node_name(bsal_worker_pool_get_node(scheduler->pool)), actor_name,
                    old_worker, new_worker);

    old_worker_object = bsal_worker_pool_get_worker(scheduler->pool, old_worker);
    new_worker_object = bsal_worker_pool_get_worker(scheduler->pool, new_worker);

    /* evict the actor from the old worker
     */
    bsal_worker_evict_actor(old_worker_object, actor_name);

    /* Redirect messages for this actor to the
     * new worker
     */
    bsal_map_update_value(&scheduler->actor_affinities, &actor_name, &new_worker);

#ifdef BSAL_WORKER_POOL_DEBUG_MIGRATION
    printf("ROUTE actor %d ->  worker %d\n", actor_name, new_worker);
#endif

    bsal_worker_enqueue_actor_special(new_worker_object, &actor);

}

int bsal_scheduler_get_actor_production(struct bsal_scheduler *scheduler, struct bsal_actor *actor)
{
    int messages;
    int last_messages;
    int name;
    int result;

    if (actor == NULL) {
        return 0;
    }

    messages = bsal_actor_get_sum_of_received_messages(actor);

    last_messages = 0;
    name = bsal_actor_name(actor);
    bsal_map_get_value(&scheduler->last_actor_received_messages, &name, &last_messages);

    result = messages - last_messages;

    return result;
}

void bsal_scheduler_update_actor_production(struct bsal_scheduler *scheduler, struct bsal_actor *actor)
{
    int messages;
    int name;

    if (actor == NULL) {
        return;
    }

    messages = bsal_actor_get_sum_of_received_messages(actor);
    name = bsal_actor_name(actor);

    if (!bsal_map_update_value(&scheduler->last_actor_received_messages, &name, &messages)) {
        bsal_map_add_value(&scheduler->last_actor_received_messages, &name, &messages);
    }
}

int bsal_scheduler_get_actor_worker(struct bsal_scheduler *scheduler, int name)
{
    int worker_index;

    worker_index = -1;

    bsal_map_get_value(&scheduler->actor_affinities, &name, &worker_index);

    return worker_index;
}


void bsal_scheduler_set_actor_worker(struct bsal_scheduler *scheduler, int name, int worker_index)
{
    bsal_map_add_value(&scheduler->actor_affinities, &name, &worker_index);
}

#ifdef BSAL_WORKER_HAS_OWN_QUEUES
int bsal_scheduler_select_worker_least_busy(
                struct bsal_scheduler *scheduler, struct bsal_message *message, int *worker_score)
{
    int to_check;
    int score;
    int best_score;
    struct bsal_worker *worker;
    struct bsal_worker *best_worker;
    int selected_worker;

#if 0
    int last_worker_score;
#endif

#ifdef BSAL_WORKER_DEBUG
    int tag;
    int destination;
    struct bsal_message *message;
#endif

    best_worker = NULL;
    best_score = 99;

    to_check = BSAL_SCHEDULER_WORK_SCHEDULING_WINDOW;

    while (to_check--) {

        /*
         * get the worker to test for this iteration.
         */
        worker = bsal_worker_pool_get_worker(scheduler->pool, scheduler->worker_for_work);

        score = bsal_worker_get_queued_messages(worker);

#ifdef BSAL_WORKER_POOL_DEBUG_ISSUE_334
        if (score >= BSAL_WORKER_WARNING_THRESHOLD
                        && (self->last_scheduling_warning == 0
                             || score >= self->last_scheduling_warning + BSAL_WORKER_WARNING_THRESHOLD_STRIDE)) {
            printf("Warning: node %d worker %d has a scheduling score of %d\n",
                            bsal_node_name(bsal_worker_pool_get_node(scheduler->pool)),
                            scheduler->worker_for_work, score);

            self->last_scheduling_warning = score;
        }
#endif

        /* if the worker is not busy and it has no work to do,
         * select it right away...
         */
        if (score == 0) {
            best_worker = worker;
            best_score = 0;
            break;
        }

        /* Otherwise, test the worker
         */
        if (best_worker == NULL || score < best_score) {
            best_worker = worker;
            best_score = score;
        }

        /*
         * assign the next worker
         */
        scheduler->worker_for_work = bsal_worker_pool_next_worker(scheduler->pool, scheduler->worker_for_work);
    }

#ifdef BSAL_WORKER_POOL_DEBUG
    message = bsal_work_message(work);
    tag = bsal_message_tag(message);
    destination = bsal_message_destination(message);

    if (tag == BSAL_ACTOR_ASK_TO_STOP) {
        printf("DEBUG dispatching BSAL_ACTOR_ASK_TO_STOP for actor %d to worker %d\n",
                        destination, *start);
    }


#endif

    selected_worker = scheduler->worker_for_work;

    /*
     * assign the next worker
     */
    scheduler->worker_for_work = bsal_worker_pool_next_worker(scheduler->pool, scheduler->worker_for_work);

    *worker_score = best_score;
    /* This is a best effort algorithm
     */
    return selected_worker;
}

#endif


