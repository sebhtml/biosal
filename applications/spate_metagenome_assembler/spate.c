
#include "spate.h"

#include <genomics/assembly/assembly_graph_store.h>
#include <genomics/assembly/assembly_sliding_window.h>
#include <genomics/assembly/assembly_block_classifier.h>
#include <genomics/assembly/assembly_graph_builder.h>
#include <genomics/assembly/assembly_arc_kernel.h>
#include <genomics/assembly/assembly_arc_classifier.h>

#include <genomics/input/input_controller.h>

#include <stdio.h>
#include <string.h>

struct bsal_script spate_script = {
    .identifier = SPATE_SCRIPT,
    .init = spate_init,
    .destroy = spate_destroy,
    .receive = spate_receive,
    .size = sizeof(struct spate),
    .name = "spate",
    .version = "0.0.1-development",
    .author = "Sébastien Boisvert",
    .description = "Exact, convenient, and scalable metagenome assembly and genome isolation for everyone"
};

void spate_init(struct bsal_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    bsal_vector_init(&concrete_self->initial_actors, sizeof(int));
    bsal_vector_init(&concrete_self->sequence_stores, sizeof(int));

    concrete_self->is_leader = 0;

    concrete_self->input_controller = BSAL_ACTOR_NOBODY;
    concrete_self->manager_for_sequence_stores = BSAL_ACTOR_NOBODY;
    concrete_self->assembly_graph = BSAL_ACTOR_NOBODY;
    concrete_self->assembly_graph_builder = BSAL_ACTOR_NOBODY;

    bsal_actor_add_route(self,
                    BSAL_ACTOR_START, spate_start);
    bsal_actor_add_route(self,
                    BSAL_ACTOR_ASK_TO_STOP, spate_ask_to_stop);
    bsal_actor_add_route(self,
                    BSAL_ACTOR_SPAWN_REPLY, spate_spawn_reply);
    bsal_actor_add_route(self,
                    BSAL_MANAGER_SET_SCRIPT_REPLY, spate_set_script_reply);
    bsal_actor_add_route(self,
                    BSAL_ACTOR_SET_CONSUMERS_REPLY, spate_set_consumers_reply);
    bsal_actor_add_route(self,
                    BSAL_SET_BLOCK_SIZE_REPLY, spate_set_block_size_reply);
    bsal_actor_add_route(self,
                    BSAL_INPUT_DISTRIBUTE_REPLY, spate_distribute_reply);
    bsal_actor_add_route(self,
                    SPATE_ADD_FILES, spate_add_files);
    bsal_actor_add_route(self,
                    SPATE_ADD_FILES_REPLY, spate_add_files_reply);
    bsal_actor_add_route(self,
                    BSAL_ADD_FILE_REPLY, spate_add_file_reply);

    bsal_actor_add_route(self,
                    BSAL_ACTOR_START_REPLY, spate_start_reply);

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

    bsal_actor_add_script(self, BSAL_ASSEMBLY_GRAPH_STORE_SCRIPT,
                    &bsal_assembly_graph_store_script);
    bsal_actor_add_script(self, BSAL_ASSEMBLY_SLIDING_WINDOW_SCRIPT,
                    &bsal_assembly_sliding_window_script);
    bsal_actor_add_script(self, BSAL_ASSEMBLY_BLOCK_CLASSIFIER_SCRIPT,
                    &bsal_assembly_block_classifier_script);
    bsal_actor_add_script(self, BSAL_COVERAGE_DISTRIBUTION_SCRIPT,
                    &bsal_coverage_distribution_script);

    bsal_actor_add_script(self, BSAL_ASSEMBLY_ARC_KERNEL_SCRIPT,
                    &bsal_assembly_arc_kernel_script);
    bsal_actor_add_script(self, BSAL_ASSEMBLY_ARC_CLASSIFIER_SCRIPT,
                    &bsal_assembly_arc_classifier_script);

    concrete_self->block_size = 16 * 4096;

    concrete_self->file_index = 0;
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
    bsal_vector_destroy(&concrete_self->sequence_stores);
}

void spate_receive(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_use_route(self, message);
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

    spawner = bsal_actor_get_spawner(self, &concrete_self->initial_actors);

    bsal_actor_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_INPUT_CONTROLLER_SCRIPT);
}

void spate_ask_to_stop(struct bsal_actor *self, struct bsal_message *message)
{
    int source;
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    source = bsal_message_source(message);

    if (!spate_must_print_help(self)) {
        printf("spate %d stops\n", bsal_actor_name(self));
    }

    bsal_actor_ask_to_stop(self, message);

    /*
     * Check if the source is an initial actor because each initial actor
     * is its own supervisor.
     */

    if (bsal_vector_index_of(&concrete_self->initial_actors, &source) >= 0) {
        bsal_actor_send_to_self_empty(self, BSAL_ACTOR_STOP);
    }

    if (concrete_self->is_leader) {

        bsal_actor_send_empty(self, concrete_self->assembly_graph_builder,
                        BSAL_ACTOR_ASK_TO_STOP);
        bsal_actor_send_empty(self, concrete_self->manager_for_sequence_stores,
                        BSAL_ACTOR_ASK_TO_STOP);
    }
}

void spate_spawn_reply(struct bsal_actor *self, struct bsal_message *message)
{
    int new_actor;
    int spawner;
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    bsal_message_unpack_int(message, 0, &new_actor);

    if (concrete_self->input_controller == BSAL_ACTOR_NOBODY) {

        concrete_self->input_controller = new_actor;

        bsal_actor_add_route_with_source(self,
                    BSAL_ACTOR_START_REPLY,
                    spate_start_reply_controller,
                    concrete_self->input_controller);

        printf("spate %d spawned controller %d\n", bsal_actor_name(self),
                        new_actor);

        spawner = bsal_actor_get_spawner(self, &concrete_self->initial_actors);

        bsal_actor_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_MANAGER_SCRIPT);

    } else if (concrete_self->manager_for_sequence_stores == BSAL_ACTOR_NOBODY) {

        concrete_self->manager_for_sequence_stores = new_actor;

        bsal_actor_add_route_with_source(self,
                    BSAL_ACTOR_START_REPLY,
                    spate_start_reply_manager,
                    concrete_self->manager_for_sequence_stores);

        printf("spate %d spawned manager %d\n", bsal_actor_name(self),
                        new_actor);

        spawner = bsal_actor_get_spawner(self, &concrete_self->initial_actors);

        bsal_actor_send_int(self, spawner, BSAL_ACTOR_SPAWN, BSAL_ASSEMBLY_GRAPH_BUILDER_SCRIPT);

    } else if (concrete_self->assembly_graph_builder == BSAL_ACTOR_NOBODY) {

        concrete_self->assembly_graph_builder = new_actor;

        bsal_actor_add_route_with_source(self,
                    BSAL_ACTOR_START_REPLY,
                    spate_start_reply_builder,
                concrete_self->assembly_graph_builder);

        bsal_actor_add_route_with_source(self,
                    BSAL_ACTOR_SET_PRODUCERS_REPLY,
                    spate_set_producers_reply,
                    concrete_self->assembly_graph_builder);

        printf("spate %d spawned graph builder %d\n", bsal_actor_name(self),
                        new_actor);

        bsal_actor_send_int(self, concrete_self->manager_for_sequence_stores, BSAL_MANAGER_SET_SCRIPT,
                        BSAL_SEQUENCE_STORE_SCRIPT);
    }
}

void spate_set_script_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    bsal_actor_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->initial_actors);
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

    bsal_actor_send_vector(self, concrete_self->input_controller, BSAL_ACTOR_SET_CONSUMERS, &consumers);

    bsal_vector_push_back_vector(&concrete_self->sequence_stores, &consumers);

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

    bsal_actor_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->initial_actors);
}

void spate_start_reply_controller(struct bsal_actor *self, struct bsal_message *message)
{
    struct spate *concrete_self;

    printf("received reply from controller\n");
    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    /*
     * Stop the actor computation
     */

    bsal_actor_send_reply_int(self, BSAL_SET_BLOCK_SIZE, concrete_self->block_size);
}

void spate_set_block_size_reply(struct bsal_actor *self, struct bsal_message *message)
{

    bsal_actor_send_to_self_empty(self, SPATE_ADD_FILES);
}

void spate_add_files(struct bsal_actor *self, struct bsal_message *message)
{
    if (!spate_add_file(self)) {
        bsal_actor_send_to_self_empty(self, SPATE_ADD_FILES_REPLY);
    }
}

void spate_add_files_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    bsal_actor_send_empty(self, concrete_self->input_controller,
                    BSAL_INPUT_DISTRIBUTE);
}

void spate_distribute_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    printf("spate %d: all sequence stores are ready\n",
                    bsal_actor_name(self));

    bsal_actor_send_vector(self, concrete_self->assembly_graph_builder,
                    BSAL_ACTOR_SET_PRODUCERS, &concrete_self->sequence_stores);

    /* kill the controller
     */

    bsal_actor_send_reply_empty(self, BSAL_ACTOR_ASK_TO_STOP);
}

void spate_set_producers_reply(struct bsal_actor *self, struct bsal_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);

    bsal_actor_send_reply_vector(self, BSAL_ACTOR_START, &concrete_self->initial_actors);
}

void spate_start_reply_builder(struct bsal_actor *self, struct bsal_message *message)
{
    spate_stop(self);
}

int spate_add_file(struct bsal_actor *self)
{
    char *file;
    int argc;
    char **argv;
    struct bsal_message message;
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    argc = bsal_actor_argc(self);
    argv = bsal_actor_argv(self);

    if (concrete_self->file_index < argc) {
        file = argv[concrete_self->file_index];

        bsal_message_init(&message, BSAL_ADD_FILE, strlen(file) + 1, file);

        bsal_actor_send(self, concrete_self->input_controller, &message);

        bsal_message_destroy(&message);

        ++concrete_self->file_index;

        return 1;
    }

    return 0;
}

void spate_add_file_reply(struct bsal_actor *self, struct bsal_message *message)
{
    bsal_actor_send_to_self_empty(self, SPATE_ADD_FILES);
}

void spate_help(struct bsal_actor *self)
{
    printf("Name: %s\n", bsal_actor_script_name(self));
    printf("Description: %s\n", bsal_script_description(bsal_actor_get_script(self)));
    printf("Version: %s\n", bsal_script_version(bsal_actor_get_script(self)));
    printf("Library: biosal (biological sequence actor library)\n");
    printf("Engine: thorium (distributed event-driven native actor machine emulator)\n");

    printf("\n");
    printf("Usage:\n");

    printf("mpiexec -n <ranks> spate -threads-per-node <threads> [-k <kmer_length>] [-i <file>] [-p <file1> <file2>] [-s <file>] -o <output>\n");

    printf("\n");
    printf("Default values: -k %d -threads-per-node %d -o %s\n",
                    BSAL_ASSEMBLY_GRAPH_BUILDER_DEFAULT_KMER_LENGTH,
                    1,
                    BSAL_COVERAGE_DISTRIBUTION_DEFAULT_OUTPUT);

    printf("\n");
    printf("Example:\n");
    printf("mpiexec -n 128 spate -threads-per-node 24 -k 51 -i interleaved_file_1.fastq -i interleaved_file_2.fastq -o my-assembly\n");

    printf("\n");
    printf("Supported input formats: .fastq (upcoming: .fasta, .fastq.gz)\n");

}

void spate_stop(struct bsal_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)bsal_actor_concrete_actor(self);
    bsal_actor_send_range_empty(self, &concrete_self->initial_actors, BSAL_ACTOR_ASK_TO_STOP);
}

int spate_must_print_help(struct bsal_actor *self)
{
    int argc;
    char **argv;
    int i;

    argc = bsal_actor_argc(self);

    if (argc == 1) {
        return 1;
    }

    argv = bsal_actor_argv(self);

    for (i = 0; i < argc; i++) {
        if (strstr(argv[i], "help") != NULL
                        || strstr(argv[i], "version") != NULL) {
            return 1;
        }
    }

    return 0;
}
