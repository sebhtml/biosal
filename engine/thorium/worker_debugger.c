
#include "worker_debugger.h"

#include "worker.h"
#include "node.h"

void thorium_worker_print_actors(struct thorium_worker *worker, struct thorium_balancer *scheduler)
{
    struct core_map_iterator iterator;
    int name;
    int count;
    struct thorium_actor *actor;
    int producers;
    int consumers;
    int received;
    int difference;
    int script;
    struct core_map distribution;
    int frequency;
    struct thorium_script *script_object;
    int dead;
    int node_name;
    int worker_name;
    int previous_amount;

    node_name = thorium_node_name(worker->node);
    worker_name = worker->name;

    core_map_iterator_init(&iterator, &worker->actors);

    printf("node/%d worker/%d %d queued messages, received: %d busy: %d load: %f ring: %d scheduled actors: %d/%d\n",
                    node_name, worker_name,
                    thorium_worker_get_scheduled_message_count(worker),
                    thorium_worker_get_sum_of_received_actor_messages(worker),
                    thorium_worker_is_busy(worker),
                    thorium_worker_get_scheduling_epoch_load(worker),
                    core_fast_ring_size_from_producer(&worker->input_actor_ring),
                    thorium_scheduler_size(&worker->scheduler),
                    (int)core_map_size(&worker->actors));

    core_map_init(&distribution, sizeof(int), sizeof(int));

    while (core_map_iterator_get_next_key_and_value(&iterator, &name, NULL)) {

        actor = thorium_node_get_actor_from_name(worker->node, name);

        if (actor == NULL) {
            continue;
        }

        dead = thorium_actor_dead(actor);

        if (dead) {
            continue;
        }

        count = thorium_actor_get_mailbox_size(actor);
        received = thorium_actor_get_sum_of_received_messages(actor);
        producers = core_map_size(thorium_actor_get_received_messages(actor));
        consumers = core_map_size(thorium_actor_get_sent_messages(actor));
        previous_amount = 0;

        core_map_get_value(&worker->actor_received_messages, &name,
                        &previous_amount);
        difference = received - previous_amount;;

        if (!core_map_update_value(&worker->actor_received_messages, &name,
                        &received)) {
            core_map_add_value(&worker->actor_received_messages, &name, &received);
        }

        printf("  [%s/%d] mailbox: %d received: %d (+%d) producers: %d consumers: %d\n",
                        thorium_actor_script_name(actor),
                        name, count, received,
                       difference,
                       producers, consumers);

        script = thorium_actor_script(actor);

        if (core_map_get_value(&distribution, &script, &frequency)) {
            ++frequency;
            core_map_update_value(&distribution, &script, &frequency);
        } else {
            frequency = 1;
            core_map_add_value(&distribution, &script, &frequency);
        }
    }

    /*printf("\n");*/
    core_map_iterator_destroy(&iterator);

    core_map_iterator_init(&iterator, &distribution);

    printf("node/%d worker/%d Frequency list\n", node_name, worker_name);

    while (core_map_iterator_get_next_key_and_value(&iterator, &script, &frequency)) {

        script_object = thorium_node_find_script(worker->node, script);

        CORE_DEBUGGER_ASSERT(script_object != NULL);

        printf("node/%d worker/%d Frequency %s => %d\n",
                        node_name,
                        worker->name,
                        thorium_script_name(script_object),
                        frequency);
    }

    core_map_iterator_destroy(&iterator);
    core_map_destroy(&distribution);
}


