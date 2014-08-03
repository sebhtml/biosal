
#include "spate.h"

#include <genomics/input/input_controller.h>

#include <stdio.h>

struct bsal_script spate_script = {
    .identifier = SPATE_SCRIPT,
    .init = spate_init,
    .destroy = spate_destroy,
    .receive = spate_receive,
    .size = sizeof(struct spate),
    .name = "spate",
    .version = "Project Thor",
    .author = "Sébastien Boisvert",
    .description = "Exact, convenient, and scalable metagenome assembly and genome isolation for everyone"
};

void spate_init(struct bsal_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    bsal_vector_init(&concrete_self->initial_actors, sizeof(int));

    concrete_self->is_leader = 0;

    concrete_self->input_controller = BSAL_ACTOR_NOBODY;
    concrete_self->manager_for_sequence_stores = BSAL_ACTOR_NOBODY;
    concrete_self->assembly_graph = BSAL_ACTOR_NOBODY;
    concrete_self->assembly_graph_builder = BSAL_ACTOR_NOBODY;

    bsal_actor_register_handler(self, BSAL_ACTOR_START, spate_start);
    bsal_actor_register_handler(self, BSAL_ACTOR_ASK_TO_STOP, spate_ask_to_stop);
    bsal_actor_register_handler(self, BSAL_ACTOR_SPAWN_REPLY, spate_spawn_reply);
    bsal_actor_register_handler(self, BSAL_MANAGER_SET_SCRIPT_REPLY, spate_set_script_reply);
    bsal_actor_register_handler(self, BSAL_ACTOR_SET_CONSUMERS_REPLY, spate_set_consumers_reply);

    bsal_actor_register_handler(self, BSAL_ACTOR_START_REPLY, spate_start_reply);
    bsal_actor_register_handler_with_source(self, BSAL_ACTOR_START_REPLY, &concrete_self->manager_for_sequence_stores, spate_start_reply_manager);
    bsal_actor_register_handler_with_source(self, BSAL_ACTOR_START_REPLY, &concrete_self->input_controller, spate_start_reply_controller);

    /*
     * Register required actor scripts now
     */

    bsal_actor_add_script(self, BSAL_INPUT_CONTROLLER_SCRIPT,
                    &bsal_input_controller_script);
    bsal_actor_add_script(self, BSAL_DNA_KMER_COUNTER_KERNEL_SCRIPT,
                    &bsal_dna_kmer_counter_kernel_script);
    bsal_actor_add_script(self, BSAL_MANAGER_SCRIPT,
                    &bsal_manager_script);
    bsal_actor_add_script(self, BSAL_AGGREGATOR_SCRIPT,
                    &bsal_aggregator_script);
    bsal_actor_add_script(self, BSAL_KMER_STORE_SCRIPT,
                    &bsal_kmer_store_script);
    bsal_actor_add_script(self, BSAL_SEQUENCE_STORE_SCRIPT,
                    &bsal_sequence_store_script);
    bsal_actor_add_script(self, BSAL_COVERAGE_DISTRIBUTION_SCRIPT,
                    &bsal_coverage_distribution_script);
    bsal_actor_add_script(self, BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT,
                    &bsal_assembly_graph_builder_script);
}

void spate_destroy(struct bsal_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    concrete_self->input_controller = BSAL_ACTOR_NOBODY;
    concrete_self->manager_for_sequence_stores = BSAL_ACTOR_NOBODY;
    concrete_self->assembly_graph = BSAL_ACTOR_NOBODY;
    concrete_self->assembly_graph_builder = BSAL_ACTOR_NOBODY;

    bsal_vector_destroy(&concrete_self->initial_actors);
}

void spate_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_call_handler(self, message);
}

void spate_start(struct bsal_actor *self, struct bsal_message *message)
{
    void *buffer;
    int name;
    struct spate *concrete_self;
    int spawner;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    buffer = bsal_message_buffer(message);
    name = bsal_actor_name(self);

    /*
     * The buffer contains initial actors spawned by Thorium
     */
    bsal_vector_unpack(&concrete_self->initial_actors, buffer);

    concrete_self->spawner_index = bsal_vector_size(&concrete_self->initial_actors) - 1;

    printf("spate/%d starts\n", name);

    if (bsal_vector_index_of(&concrete_self->initial_actors, &name) == 0) {
        concrete_self->is_leader = 1;
    }

    /*
     * Abort if the actor is not the leader of the tribe.
     */
    if (!concrete_self->is_leader) {
        return;
    }

    spawner = spate_get_spawner(self);

    bsal_actor_helper_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_INPUT_CONTROLLER_SCRIPT);
}

void spate_ask_to_stop(struct bsal_actor *self, struct bsal_message *message)
{
    int source;
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    source = bsal_message_source(message);

    printf("spate %d stops\n", bsal_actor_name(self));

    bsal_actor_helper_ask_to_stop(self, message);

    /*
     * Check if the source is an initial actor because each initial actor
     * is its own supervisor.
     */

    if (bsal_vector_index_of(&concrete_self->initial_actors, &source) >= 0) {
        bsal_actor_helper_send_to_self_empty(self, BSAL_ACTOR_STOP);
    }
}

void spate_spawn_reply(struct bsal_actor *self, struct bsal_message *message)
{
    int new_actor;
    int spawner;
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    bsal_message_helper_unpack_int(message, 0, &new_actor);

    if (concrete_self->input_controller == BSAL_ACTOR_NOBODY) {

        concrete_self->input_controller = new_actor;

        printf("spate %d spawned controller %d\n", bsal_actor_name(self),
                        new_actor);

        spawner = spate_get_spawner(self);
        bsal_actor_helper_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_MANAGER_SCRIPT);

    } else if (concrete_self->manager_for_sequence_stores == BSAL_ACTOR_NOBODY) {

        concrete_self->manager_for_sequence_stores = new_actor;

        printf("spate %d spawned manager %d\n", bsal_actor_name(self),
                        new_actor);

        spawner = spate_get_spawner(self);
        bsal_actor_helper_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT);

    } else if (concrete_self->assembly_graph_builder == BSAL_ACTOR_NOBODY) {

        concrete_self->assembly_graph_builder = new_actor;

        printf("spate %d spawned graph builder %d\n", bsal_actor_name(self),
                        new_actor);

        bsal_actor_helper_send_int(self, concrete_self->manager_for_sequence_stores, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_SEQUENCE_STORE_SCRIPT);
    }
}

int spate_get_spawner(struct bsal_actor *self)
{
    int actor;
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    actor = bsal_vector_helper_at_as_int(&concrete_self->initial_actors, concrete_self->spawner_index);

    --concrete_self->spawner_index;

    if (concrete_self->spawner_index < 0) {

        concrete_self->spawner_index = bsal_vector_size(&concrete_self->initial_actors) - 1;
    }

    return actor;
}

void spate_set_script_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    bsal_actor_helper_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->initial_actors);
}

void spate_start_reply(struct bsal_actor *self, struct bsal_message *message)
{
}

void spate_start_reply_manager(struct bsal_actor *self, struct bsal_message *message)
{
    struct bsal_vector consumers;
    struct spate *concrete_self;
    void *buffer;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    bsal_vector_init(&consumers, sizeof(int));

    buffer = bsal_message_buffer(message);

    bsal_vector_unpack(&consumers, buffer);

    printf("spate %d sends the names of %d consumers to controller %d\n",
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&consumers),
                    concrete_self->input_controller);

    bsal_actor_helper_send_vector(self, concrete_self->input_controller, BSAL_ACTOR_SET_CONSUMERS, &consumers);

    bsal_vector_destroy(&consumers);

}

void spate_set_consumers_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    printf("spate %d sends %d spawners to controller %d\n",
                    bsal_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->initial_actors),
                    concrete_self->input_controller);

    bsal_actor_helper_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->initial_actors);
}

void spate_start_reply_controller(struct bsal_actor *self, struct bsal_message *message)
{
    struct spate *concrete_self;

    printf("received reply from controller\n");
    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    /*
     * Stop the actor computation
     */


    bsal_actor_helper_send_range_empty(self, &concrete_self->initial_actors, BSAL_ACTOR_ASK_TO_STOP);
}
