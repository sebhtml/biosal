
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

#include <core/system/timer.h>

#include <stdio.h>

/* Parameters for the scheduler
 */
#define SCHEDULER_PRECISION 1000000

/*
 * Use the 10th percentile and the 90th percentile
 */
#define SCHEDULER_WINDOW 10

#define BSAL_SCHEDULER_WORK_SCHEDULING_WINDOW 8192 * 4

/*
 * Definitions for scheduling classes
 * See bsal_worker_pool_balance for classification requirements.
 */

#define BSAL_CLASS_STALLED_STRING "BSAL_CLASS_STALLED"
#define BSAL_CLASS_OPERATING_AT_FULL_CAPACITY_STRING "BSAL_CLASS_OPERATING_AT_FULL_CAPACITY"
#define BSAL_CLASS_NORMAL_STRING "BSAL_CLASS_NORMAL"
#define BSAL_CLASS_HUB_STRING "BSAL_CLASS_HUB"
#define BSAL_CLASS_BURDENED_STRING "BSAL_CLASS_BURDENED"

/*
*/
#define BSAL_SCHEDULER_ENABLE_SYMMETRIC_SCHEDULING

void bsal_scheduler_init(struct bsal_scheduler *scheduler, struct bsal_worker_pool *pool)
{
    scheduler->pool = pool;
    bsal_map_init(&scheduler->actor_affinities, sizeof(int), sizeof(int));
    bsal_map_init(&scheduler->last_actor_received_messages, sizeof(int), sizeof(int));

    scheduler->worker_for_work = 0;
    scheduler->last_migrations = 0;
    scheduler->last_killed_actors = -1;
    scheduler->last_spawned_actors = -1;
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
    struct bsal_timer timer;

    int i;
    struct bsal_vector loads;
    struct bsal_vector loads_unsorted;
    struct bsal_vector burdened_workers;
    struct bsal_vector stalled_workers;
    struct bsal_worker *worker;
    struct bsal_node *node;

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
    int spawned_actors;
    int killed_actors;
    int perfect;

#ifdef BSAL_SCHEDULER_ENABLE_SYMMETRIC_SCHEDULING
    struct bsal_map symmetric_actor_scripts;
    int script;
#endif

    node = bsal_worker_pool_get_node(scheduler->pool);

    spawned_actors = bsal_node_get_counter(node, BSAL_COUNTER_SPAWNED_ACTORS);

    /* There is nothing to balance...
     */
    if (spawned_actors == 0) {
        return;
    }

    killed_actors = bsal_node_get_counter(node, BSAL_COUNTER_KILLED_ACTORS);

    /*
     * The system can probably not be balanced to get in
     * a better shape anyway.
     */
    if (spawned_actors == scheduler->last_spawned_actors
                    && killed_actors == scheduler->last_killed_actors
                    && scheduler->last_migrations == 0) {

        printf("SCHEDULER: balance can not be improved because nothing changed.\n");
        return;
    }

    /* Check if we have perfection
     */

    perfect = 1;
    for (i = 0; i < bsal_worker_pool_worker_count(scheduler->pool); i++) {
        worker = bsal_worker_pool_get_worker(scheduler->pool, i);

        load_value = bsal_worker_get_epoch_load(worker) * 100;

        if (load_value != 100) {
            perfect = 0;
            break;
        }
    }

    if (perfect) {
        printf("SCHEDULER: perfect balance can not be improved.\n");
        return;
    }

    /* update counters
     */
    scheduler->last_spawned_actors = spawned_actors;
    scheduler->last_killed_actors = killed_actors;

    /* Otherwise, try to balance things
     */
    bsal_timer_init(&timer);

    bsal_timer_start(&timer);

#ifdef BSAL_SCHEDULER_ENABLE_SYMMETRIC_SCHEDULING
    bsal_map_init(&symmetric_actor_scripts, sizeof(int), sizeof(int));

    bsal_scheduler_detect_symmetric_scripts(scheduler, &symmetric_actor_scripts);
#endif

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

            if (actor == NULL) {
                continue;
            }
            messages = bsal_scheduler_get_actor_production(scheduler, actor);

#ifdef BSAL_SCHEDULER_ENABLE_SYMMETRIC_SCHEDULING
            script = bsal_actor_script(actor);


            /* symmetric actors are migrated elsewhere.
             */
            if (bsal_map_get_value(&symmetric_actor_scripts, &script, NULL)) {
                continue;
            }
#endif

            /* Simulate the remaining load
             */
            projected_load = remaining_load;
            projected_load -= ((0.0 + messages) / total) * load_value;

#ifdef BSAL_SCHEDULER_DEBUG
            printf(" TESTING actor %d, production was %d, projected_load is %d (- %d * (1 - %d/%d)\n",
                            actor_name, messages, projected_load,
                            load_value, messages, total);
#endif

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

#ifdef BSAL_SCHEDULER_DEBUG
                printf("early CANDIDATE for migration: actor %d, worker %d\n",
                                actor_name, old_worker);
#endif
            }
        }
        bsal_map_iterator_destroy(&set_iterator);

    }

    bsal_vector_iterator_destroy(&vector_iterator);

    /* Sort the candidates
     */

    /*
    bsal_vector_helper_sort_int(&actors_to_migrate);

    printf("Percentiles for production: ");
    bsal_statistics_get_print_percentiles_int(&actors_to_migrate);
    */

    /* Sort them in reverse order.
     */
    bsal_vector_helper_sort_int_reverse(&actors_to_migrate);

    bsal_vector_iterator_init(&vector_iterator, &actors_to_migrate);

    /* For each highly active actor,
     * try to match it with a stalled worker
     */
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
#ifdef BSAL_SCHEDULER_DEBUG
                printf("Scheduler: skipping actor %d, predicted load is %d >= 100\n",
                           actor_name, predicted_new_load);
#endif

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

#ifdef BSAL_SCHEDULER_ENABLE_SYMMETRIC_SCHEDULING
    /* Generate migrations for symmetric actors.
     */

    bsal_scheduler_generate_symmetric_migrations(scheduler, &symmetric_actor_scripts, &migrations);
#endif

    /* Actually do the migrations
     */
    bsal_vector_iterator_init(&vector_iterator, &migrations);

    while (bsal_vector_iterator_next(&vector_iterator, (void **)&migration_to_do)) {

        bsal_scheduler_migrate(scheduler, migration_to_do);
    }

    bsal_vector_iterator_destroy(&vector_iterator);

    scheduler->last_migrations = bsal_vector_size(&migrations);

    bsal_vector_destroy(&migrations);

    /* Unlock all workers
     */
    for (i = 0; i < bsal_worker_pool_worker_count(scheduler->pool); i++) {
        worker = bsal_worker_pool_get_worker(scheduler->pool, i);

        bsal_worker_unlock(worker);
    }

#ifdef BSAL_SCHEDULER_ENABLE_SYMMETRIC_SCHEDULING
    bsal_map_destroy(&symmetric_actor_scripts);
#endif

    bsal_timer_stop(&timer);

    printf("SCHEDULER: elapsed time for balancing: %d us, %d migrations performed\n",
                    (int)(bsal_timer_get_elapsed_nanoseconds(&timer) / 1000),
                    scheduler->last_migrations);
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
    name = bsal_actor_get_name(actor);
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
    name = bsal_actor_get_name(actor);

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

void bsal_scheduler_detect_symmetric_scripts(struct bsal_scheduler *scheduler, struct bsal_map *symmetric_actor_scripts)
{
    int i;
    struct bsal_worker *worker;
    struct bsal_actor *actor;
    struct bsal_map_iterator iterator;
    struct bsal_map *set;
    int actor_name;
    struct bsal_node *node;
    int script;
    int frequency;
    struct bsal_map frequencies;
    int worker_count;
    int population_per_worker;
    struct bsal_script *actual_script;

    worker_count = bsal_worker_pool_worker_count(scheduler->pool);
    bsal_map_init(&frequencies, sizeof(int), sizeof(int));

    node = bsal_worker_pool_get_node(scheduler->pool);

    /* Gather frequencies
     */
    for (i = 0; i < worker_count; i++) {

        worker = bsal_worker_pool_get_worker(scheduler->pool, i);

        set = bsal_worker_get_actors(worker);

        bsal_map_iterator_init(&iterator, set);

        while (bsal_map_iterator_get_next_key_and_value(&iterator, &actor_name, NULL)) {
            actor = bsal_node_get_actor_from_name(node, actor_name);

            if (actor == NULL) {
                continue;
            }
            script = bsal_actor_script(actor);

            frequency = 0;

            if (!bsal_map_get_value(&frequencies, &script, &frequency)) {
                bsal_map_add_value(&frequencies, &script, &frequency);
            }

            ++frequency;

            bsal_map_update_value(&frequencies, &script, &frequency);
        }

        bsal_map_iterator_destroy(&iterator);
    }

    /*
     * Detect symmetric scripts
     */
    bsal_map_iterator_init(&iterator, &frequencies);

    while (bsal_map_iterator_get_next_key_and_value(&iterator, &script, &frequency)) {

        actual_script = bsal_node_find_script(node, script);

        printf("SCHEDULER test symmetry %s %d\n",
                        bsal_script_description(actual_script),
                        frequency);

        if (frequency % worker_count == 0) {
            population_per_worker = frequency / worker_count;

            bsal_map_add_value(symmetric_actor_scripts, &script, &population_per_worker);

            printf("SCHEDULER: script %s is symmetric, worker_count: %d, population_per_worker: %d\n",
                            bsal_script_description(actual_script),
                            worker_count,
                            population_per_worker);
        }
    }

    bsal_map_iterator_destroy(&iterator);

    bsal_map_destroy(&frequencies);
}

void bsal_scheduler_generate_symmetric_migrations(struct bsal_scheduler *scheduler, struct bsal_map *symmetric_actor_scripts,
                struct bsal_vector *migrations)
{
    int i;
    int worker_count;
    struct bsal_worker *worker;
    struct bsal_map *set;
    struct bsal_map_iterator iterator;
    struct bsal_migration migration;
    struct bsal_map script_current_worker;
    struct bsal_map script_current_worker_actor_count;
    int frequency;
    int current_worker;
    int current_worker_actor_count;
    int old_worker;
    struct bsal_script *actual_script;
    struct bsal_node *node;
    int actor_name;
    int script;
    int new_worker;
    struct bsal_actor *actor;
    int enabled;

    /* Gather symmetric actors:
     */

#ifdef BSAL_SCHEDULER_ENABLE_SYMMETRIC_SCHEDULING
    enabled = 1;
#else
    enabled = 0;
#endif

    bsal_map_init(&script_current_worker, sizeof(int), sizeof(int));
    bsal_map_init(&script_current_worker_actor_count, sizeof(int), sizeof(int));

    node = bsal_worker_pool_get_node(scheduler->pool);
    worker_count = bsal_worker_pool_worker_count(scheduler->pool);

    for (i = 0; i < worker_count; i++) {

        worker = bsal_worker_pool_get_worker(scheduler->pool, i);

        set = bsal_worker_get_actors(worker);

        bsal_map_iterator_init(&iterator, set);

        while (bsal_map_iterator_get_next_key_and_value(&iterator, &actor_name, NULL)) {
            actor = bsal_node_get_actor_from_name(node, actor_name);

            if (actor == NULL) {
                continue;
            }

            script = bsal_actor_script(actor);

            /*
             * Check if the actor is symmetric
             */
            if (bsal_map_get_value(symmetric_actor_scripts, &script, &frequency)) {

                current_worker = 0;
                if (!bsal_map_get_value(&script_current_worker, &script, &current_worker)) {
                    bsal_map_add_value(&script_current_worker, &script, &current_worker);
                }
                current_worker_actor_count = 0;
                if (!bsal_map_get_value(&script_current_worker_actor_count, &script, &current_worker_actor_count)) {
                    bsal_map_add_value(&script_current_worker_actor_count, &script, &current_worker_actor_count);
                }

                /*
                 * Emit migration instruction
                 */

                old_worker = bsal_scheduler_get_actor_worker(scheduler, actor_name);
                new_worker = current_worker;
                actual_script = bsal_node_find_script(node, script);

                if (enabled && old_worker != new_worker) {
                    bsal_migration_init(&migration, actor_name, old_worker, new_worker);
                    bsal_vector_push_back(migrations, &migration);
                    bsal_migration_destroy(&migration);

                    printf("[EMIT] ");
                } else {
                    printf("[MOCK] ");
                }

                printf("SCHEDULER -> symmetric placement... %s/%d scheduled for execution on worker/%d of node/%d\n",
                                bsal_script_description(actual_script),
                                actor_name,
                                new_worker,
                                bsal_node_name(node));

                ++current_worker_actor_count;
                bsal_map_update_value(&script_current_worker_actor_count, &script, &current_worker_actor_count);

                /* The current worker is full.
                 * Increment the current worker and set the
                 * worker actor count to 0.
                 */
                if (current_worker_actor_count == frequency) {
                    ++current_worker;
                    bsal_map_update_value(&script_current_worker, &script, &current_worker);
                    current_worker_actor_count = 0;
                    bsal_map_update_value(&script_current_worker_actor_count, &script, &current_worker_actor_count);
                }
            }

        }

        bsal_map_iterator_destroy(&iterator);
    }

    bsal_map_destroy(&script_current_worker);
    bsal_map_destroy(&script_current_worker_actor_count);
}
