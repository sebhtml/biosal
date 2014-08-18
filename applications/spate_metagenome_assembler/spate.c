
#include "spate.h"

#include <genomics/assembly/assembly_graph_store.h>
#include <genomics/assembly/assembly_sliding_window.h>
#include <genomics/assembly/assembly_block_classifier.h>
#include <genomics/assembly/assembly_graph_builder.h>
#include <genomics/assembly/assembly_arc_kernel.h>
#include <genomics/assembly/assembly_arc_classifier.h>
#include <genomics/assembly/assembly_dummy_walker.h>

#include <genomics/input/input_controller.h>

#include <stdio.h>
#include <string.h>

struct thorium_script spate_script = {
    .identifier = SPATE_SCRIPT,
    .init = spate_init,
    .destroy = spate_destroy,
    .receive = spate_receive,
    .size = sizeof(struct spate),
    .name = "spate",
    .version = "0.0.1-development",
    .author = "Sebastien Boisvert",
    .description = "Exact, convenient, and scalable metagenome assembly and genome isolation for everyone"
};

void spate_init(struct thorium_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    bsal_vector_init(&concrete_self->initial_actors, sizeof(int));
    bsal_vector_init(&concrete_self->sequence_stores, sizeof(int));
    bsal_vector_init(&concrete_self->graph_stores, sizeof(int));

    concrete_self->is_leader = 0;

    concrete_self->input_controller = THORIUM_ACTOR_NOBODY;
    concrete_self->manager_for_sequence_stores = THORIUM_ACTOR_NOBODY;
    concrete_self->assembly_graph = THORIUM_ACTOR_NOBODY;
    concrete_self->assembly_graph_builder = THORIUM_ACTOR_NOBODY;

    bsal_timer_init(&concrete_self->timer);

    thorium_actor_add_route(self,
                    THORIUM_ACTOR_START, spate_start);
    thorium_actor_add_route(self,
                    THORIUM_ACTOR_ASK_TO_STOP, spate_ask_to_stop);
    thorium_actor_add_route(self,
                    THORIUM_ACTOR_SPAWN_REPLY, spate_spawn_reply);
    thorium_actor_add_route(self,
                    BSAL_MANAGER_SET_SCRIPT_REPLY, spate_set_script_reply);
    thorium_actor_add_route(self,
                    THORIUM_ACTOR_SET_CONSUMERS_REPLY, spate_set_consumers_reply);
    thorium_actor_add_route(self,
                    BSAL_SET_BLOCK_SIZE_REPLY, spate_set_block_size_reply);
    thorium_actor_add_route(self,
                    BSAL_INPUT_DISTRIBUTE_REPLY, spate_distribute_reply);
    thorium_actor_add_route(self,
                    SPATE_ADD_FILES, spate_add_files);
    thorium_actor_add_route(self,
                    SPATE_ADD_FILES_REPLY, spate_add_files_reply);
    thorium_actor_add_route(self,
                    BSAL_ADD_FILE_REPLY, spate_add_file_reply);

    thorium_actor_add_route(self,
                    THORIUM_ACTOR_START_REPLY, spate_start_reply);

    /*
     * Register required actor scripts now
     */

    thorium_actor_add_script(self, BSAL_INPUT_CONTROLLER_SCRIPT,
                    &bsal_input_controller_script);
    thorium_actor_add_script(self, BSAL_DNA_KMER_COUNTER_KERNEL_SCRIPT,
                    &bsal_dna_kmer_counter_kernel_script);
    thorium_actor_add_script(self, BSAL_MANAGER_SCRIPT,
                    &bsal_manager_script);
    thorium_actor_add_script(self, BSAL_AGGREGATOR_SCRIPT,
                    &bsal_aggregator_script);
    thorium_actor_add_script(self, BSAL_KMER_STORE_SCRIPT,
                    &bsal_kmer_store_script);
    thorium_actor_add_script(self, BSAL_SEQUENCE_STORE_SCRIPT,
                    &bsal_sequence_store_script);
    thorium_actor_add_script(self, BSAL_COVERAGE_DISTRIBUTION_SCRIPT,
                    &bsal_coverage_distribution_script);
    thorium_actor_add_script(self, BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT,
                    &bsal_assembly_graph_builder_script);

    thorium_actor_add_script(self, BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT,
                    &bsal_assembly_graph_store_script);
    thorium_actor_add_script(self, BSAL_ASSEMBLY_SLIDING_WINDOW_SCRIPT,
                    &bsal_assembly_sliding_window_script);
    thorium_actor_add_script(self, BSAL_ASSEMBLY_BLOCK_CLASSIFIER_SCRIPT,
                    &bsal_assembly_block_classifier_script);
    thorium_actor_add_script(self, BSAL_COVERAGE_DISTRIBUTION_SCRIPT,
                    &bsal_coverage_distribution_script);

    thorium_actor_add_script(self, BSAL_ASSEMBLY_ARC_KERNEL_SCRIPT,
                    &bsal_assembly_arc_kernel_script);
    thorium_actor_add_script(self, BSAL_ASSEMBLY_ARC_CLASSIFIER_SCRIPT,
                    &bsal_assembly_arc_classifier_script);
    thorium_actor_add_script(self, BSAL_ASSEMBLY_DUMMY_WALKER_SCRIPT,
                    &bsal_assembly_dummy_walker_script);

    concrete_self->block_size = 16 * 4096;

    concrete_self->file_index = 0;
}

void spate_destroy(struct thorium_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    bsal_timer_destroy(&concrete_self->timer);

    concrete_self->input_controller = THORIUM_ACTOR_NOBODY;
    concrete_self->manager_for_sequence_stores = THORIUM_ACTOR_NOBODY;
    concrete_self->assembly_graph = THORIUM_ACTOR_NOBODY;
    concrete_self->assembly_graph_builder = THORIUM_ACTOR_NOBODY;

    bsal_vector_destroy(&concrete_self->initial_actors);
    bsal_vector_destroy(&concrete_self->sequence_stores);
    bsal_vector_destroy(&concrete_self->graph_stores);
}

void spate_receive(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_use_route(self, message);
}

void spate_start(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    int name;
    struct spate *concrete_self;
    int spawner;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);
    name = thorium_actor_name(self);

    /*
     * The buffer contains initial actors spawned by Thorium
     */
    bsal_vector_unpack(&concrete_self->initial_actors, buffer);

    if (!spate_must_print_help(self)) {
        printf("spate/%d starts\n", name);
    }

    if (bsal_vector_index_of(&concrete_self->initial_actors, &name) == 0) {
        concrete_self->is_leader = 1;
    }

    /*
     * Abort if the actor is not the leader of the tribe.
     */
    if (!concrete_self->is_leader) {
        return;
    }

    if (spate_must_print_help(self)) {
        spate_help(self);

        spate_stop(self);
        return;
    }

    bsal_timer_start(&concrete_self->timer);

    spawner = thorium_actor_get_spawner(self, &concrete_self->initial_actors);

    thorium_actor_send_int(self, spawner, THORIUM_ACTOR_SPAWN, BSAL_INPUT_CONTROLLER_SCRIPT);
}

void spate_ask_to_stop(struct thorium_actor *self, struct thorium_message *message)
{
    int source;
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    source = thorium_message_source(message);

    if (!spate_must_print_help(self)) {
        printf("spate %d stops\n", thorium_actor_name(self));
    }

#if 0
    thorium_actor_ask_to_stop(self, message);
#endif

    /*
     * Check if the source is an initial actor because each initial actor
     * is its own supervisor.
     */

    if (bsal_vector_index_of(&concrete_self->initial_actors, &source) >= 0) {
        thorium_actor_send_to_self_empty(self, THORIUM_ACTOR_STOP);
    }

    if (concrete_self->is_leader) {

        thorium_actor_send_empty(self, concrete_self->assembly_graph_builder,
                        THORIUM_ACTOR_ASK_TO_STOP);
        thorium_actor_send_empty(self, concrete_self->manager_for_sequence_stores,
                        THORIUM_ACTOR_ASK_TO_STOP);

        if (!spate_must_print_help(self)) {
            bsal_timer_stop(&concrete_self->timer);
            bsal_timer_print_with_description(&concrete_self->timer, "Run actor computation");
        }
    }
}

void spate_spawn_reply(struct thorium_actor *self, struct thorium_message *message)
{
    int new_actor;
    int spawner;
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &new_actor);

    if (concrete_self->input_controller == THORIUM_ACTOR_NOBODY) {

        concrete_self->input_controller = new_actor;

        thorium_actor_add_route_with_source(self,
                    THORIUM_ACTOR_START_REPLY,
                    spate_start_reply_controller,
                    concrete_self->input_controller);

        printf("spate %d spawned controller %d\n", thorium_actor_name(self),
                        new_actor);

        spawner = thorium_actor_get_spawner(self, &concrete_self->initial_actors);

        thorium_actor_send_int(self, spawner, THORIUM_ACTOR_SPAWN, BSAL_MANAGER_SCRIPT);

    } else if (concrete_self->manager_for_sequence_stores == THORIUM_ACTOR_NOBODY) {

        concrete_self->manager_for_sequence_stores = new_actor;

        thorium_actor_add_route_with_source(self,
                    THORIUM_ACTOR_START_REPLY,
                    spate_start_reply_manager,
                    concrete_self->manager_for_sequence_stores);

        printf("spate %d spawned manager %d\n", thorium_actor_name(self),
                        new_actor);

        spawner = thorium_actor_get_spawner(self, &concrete_self->initial_actors);

        thorium_actor_send_int(self, spawner, THORIUM_ACTOR_SPAWN, BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT);

    } else if (concrete_self->assembly_graph_builder == THORIUM_ACTOR_NOBODY) {

        concrete_self->assembly_graph_builder = new_actor;

        thorium_actor_add_route_with_source(self,
                    THORIUM_ACTOR_START_REPLY,
                    spate_start_reply_builder,
                concrete_self->assembly_graph_builder);

        thorium_actor_add_route_with_source(self,
                    THORIUM_ACTOR_SET_PRODUCERS_REPLY,
                    spate_set_producers_reply,
                    concrete_self->assembly_graph_builder);

        printf("spate %d spawned graph builder %d\n", thorium_actor_name(self),
                        new_actor);

        thorium_actor_send_int(self, concrete_self->manager_for_sequence_stores, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_SEQUENCE_STORE_SCRIPT);
    }
}

void spate_set_script_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_actor_send_reply_vector(self, THORIUM_ACTOR_START, &concrete_self->initial_actors);
}

void spate_start_reply(struct thorium_actor *self, struct thorium_message *message)
{
}

void spate_start_reply_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct bsal_vector consumers;
    struct spate *concrete_self;
    void *buffer;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    bsal_vector_init(&consumers, sizeof(int));

    buffer = thorium_message_buffer(message);

    bsal_vector_unpack(&consumers, buffer);

    printf("spate %d sends the names of %d consumers to controller %d\n",
                    thorium_actor_name(self),
                    (int)bsal_vector_size(&consumers),
                    concrete_self->input_controller);

    thorium_actor_send_vector(self, concrete_self->input_controller, THORIUM_ACTOR_SET_CONSUMERS, &consumers);

    bsal_vector_push_back_vector(&concrete_self->sequence_stores, &consumers);

    bsal_vector_destroy(&consumers);
}

void spate_set_consumers_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    printf("spate %d sends %d spawners to controller %d\n",
                    thorium_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->initial_actors),
                    concrete_self->input_controller);

    thorium_actor_send_reply_vector(self, THORIUM_ACTOR_START, &concrete_self->initial_actors);
}

void spate_start_reply_controller(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    printf("received reply from controller\n");
    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    /*
     * Stop the actor computation
     */

    thorium_actor_send_reply_int(self, BSAL_SET_BLOCK_SIZE, concrete_self->block_size);
}

void spate_set_block_size_reply(struct thorium_actor *self, struct thorium_message *message)
{

    thorium_actor_send_to_self_empty(self, SPATE_ADD_FILES);
}

void spate_add_files(struct thorium_actor *self, struct thorium_message *message)
{
    if (!spate_add_file(self)) {
        thorium_actor_send_to_self_empty(self, SPATE_ADD_FILES_REPLY);
    }
}

void spate_add_files_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_actor_send_empty(self, concrete_self->input_controller,
                    BSAL_INPUT_DISTRIBUTE);
}

void spate_distribute_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    printf("spate %d: all sequence stores are ready\n",
                    thorium_actor_name(self));

    thorium_actor_send_vector(self, concrete_self->assembly_graph_builder,
                    THORIUM_ACTOR_SET_PRODUCERS, &concrete_self->sequence_stores);

    /* kill the controller
     */

    thorium_actor_send_reply_empty(self, THORIUM_ACTOR_ASK_TO_STOP);
}

void spate_set_producers_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_actor_send_reply_vector(self, THORIUM_ACTOR_START, &concrete_self->initial_actors);
}

void spate_start_reply_builder(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    int spawner;
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    bsal_vector_unpack(&concrete_self->graph_stores, buffer);

    printf("%s/%d has %d graph stores\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)bsal_vector_size(&concrete_self->graph_stores));

    spawner = thorium_actor_get_spawner(self, &concrete_self->initial_actors);

    concrete_self->dummy_walker = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_route_with_condition(self, THORIUM_ACTOR_SPAWN_REPLY,
                    spate_spawn_reply_dummy_walker,
                    &concrete_self->dummy_walker, THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    thorium_actor_send_int(self, spawner, THORIUM_ACTOR_SPAWN, BSAL_ASSEMBLY_DUMMY_WALKER_SCRIPT);

}

void spate_spawn_reply_dummy_walker(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &concrete_self->dummy_walker);

    thorium_actor_add_route_with_source(self, THORIUM_ACTOR_START_REPLY,
                    spate_start_reply_dummy_walker,
                    concrete_self->dummy_walker);

    thorium_actor_send_vector(self, concrete_self->dummy_walker,
                    THORIUM_ACTOR_START,
                    &concrete_self->graph_stores);
}

void spate_start_reply_dummy_walker(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_send_reply_empty(self, THORIUM_ACTOR_ASK_TO_STOP);

    spate_stop(self);
}

int spate_add_file(struct thorium_actor *self)
{
    char *file;
    int argc;
    char **argv;
    struct thorium_message message;
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);

    if (concrete_self->file_index < argc) {
        file = argv[concrete_self->file_index];

        thorium_message_init(&message, BSAL_ADD_FILE, strlen(file) + 1, file);

        thorium_actor_send(self, concrete_self->input_controller, &message);

        thorium_message_destroy(&message);

        ++concrete_self->file_index;

        return 1;
    }

    return 0;
}

void spate_add_file_reply(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_send_to_self_empty(self, SPATE_ADD_FILES);
}

void spate_help(struct thorium_actor *self)
{
    printf("Application: %s\n", thorium_actor_script_name(self));
    printf("    Version: %s\n", thorium_script_version(thorium_actor_get_script(self)));
    printf("    Description: %s\n", thorium_script_description(thorium_actor_get_script(self)));
    printf("    Library: biosal (biological sequence actor library)\n");
    printf("    Engine: thorium (distributed event-driven native actor machine emulator)\n");

    printf("\n");
    printf("Usage:\n");

    printf("    mpiexec -n <ranks> spate -threads-per-node <threads> [-k <kmer_length>] [-i <file>] [-p <file1> <file2>] [-s <file>] -o <output>\n");

    printf("\n");
    printf("Default values:\n");
    printf("    -k %d (no limit and no recompilation is required)\n", BSAL_ASSEMBLY_GRAPH_BUILDER_DEFAULT_KMER_LENGTH);
    printf("    -threads-per-node %d\n", 1);
    printf("    -o %s\n",
                    BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT);

    printf("\n");
    printf("Example:\n");
    printf("    mpiexec -n 128 spate -threads-per-node 24 -k 51 -i interleaved_file_1.fastq -i interleaved_file_2.fastq -o my-assembly\n");

    printf("\n");
    printf("Supported input formats:\n");

    printf("    .fastq, .fastq.gz, .fasta, .fasta.gz\n");
    printf("    .fastq can be named .fq, fasta can be named .fa, and .fasta can be multiline.\n");

}

void spate_stop(struct thorium_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    thorium_actor_send_range_empty(self, &concrete_self->initial_actors, THORIUM_ACTOR_ASK_TO_STOP);
}

int spate_must_print_help(struct thorium_actor *self)
{
    int argc;
    char **argv;
    int i;

    argc = thorium_actor_argc(self);

    if (argc == 1) {
        return 1;
    }

    argv = thorium_actor_argv(self);

    for (i = 0; i < argc; i++) {
        if (strstr(argv[i], "-help") != NULL
                        || strstr(argv[i], "-version") != NULL) {
            return 1;
        }
    }

    return 0;
}
