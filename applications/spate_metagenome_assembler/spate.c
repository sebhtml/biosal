
#include "spate.h"

#include <genomics/assembly/assembly_graph_store.h>
#include <genomics/assembly/assembly_sliding_window.h>
#include <genomics/assembly/assembly_block_classifier.h>
#include <genomics/assembly/assembly_graph_builder.h>
#include <genomics/assembly/assembly_arc_kernel.h>
#include <genomics/assembly/assembly_arc_classifier.h>

#include <genomics/assembly/unitig/unitig_visitor.h>
#include <genomics/assembly/unitig/unitig_walker.h>
#include <genomics/assembly/unitig/unitig_manager.h>

#include <genomics/input/input_controller.h>

#include <core/system/command.h>

#include <core/file_storage/directory.h>

#include <core/patterns/manager.h>
#include <core/patterns/writer_process.h>

#include <stdio.h>
#include <string.h>

struct thorium_script spate_script = {
    .identifier = SCRIPT_SPATE,
    .init = spate_init,
    .destroy = spate_destroy,
    .receive = spate_receive,
    .size = sizeof(struct spate),
    .name = "spate",
    .version = "0.2.0-development",
    .author = "Sebastien Boisvert",
    .description = "Exact, convenient, and scalable metagenome assembly and genome isolation for everyone"
};

void spate_init(struct thorium_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    core_vector_init(&concrete_self->initial_actors, sizeof(int));
    core_vector_init(&concrete_self->sequence_stores, sizeof(int));
    core_vector_init(&concrete_self->graph_stores, sizeof(int));

    concrete_self->is_leader = 0;

    concrete_self->input_controller = THORIUM_ACTOR_NOBODY;
    concrete_self->manager_for_sequence_stores = THORIUM_ACTOR_NOBODY;
    concrete_self->assembly_graph = THORIUM_ACTOR_NOBODY;
    concrete_self->assembly_graph_builder = THORIUM_ACTOR_NOBODY;

    core_timer_init(&concrete_self->timer);

    thorium_actor_add_action(self,
                    ACTION_START, spate_start);
    thorium_actor_add_action(self,
                    ACTION_ASK_TO_STOP, spate_ask_to_stop);
    thorium_actor_add_action(self,
                    ACTION_SPAWN_REPLY, spate_spawn_reply);
    thorium_actor_add_action(self,
                    ACTION_MANAGER_SET_SCRIPT_REPLY, spate_set_script_reply);
    thorium_actor_add_action(self,
                    ACTION_SET_CONSUMERS_REPLY, spate_set_consumers_reply);
    thorium_actor_add_action(self,
                    ACTION_SET_BLOCK_SIZE_REPLY, spate_set_block_size_reply);
    thorium_actor_add_action(self,
                    ACTION_INPUT_DISTRIBUTE_REPLY, spate_distribute_reply);
    thorium_actor_add_action(self,
                    ACTION_SPATE_ADD_FILES, spate_add_files);
    thorium_actor_add_action(self,
                    ACTION_SPATE_ADD_FILES_REPLY, spate_add_files_reply);
    thorium_actor_add_action(self,
                    ACTION_ADD_FILE_REPLY, spate_add_file_reply);

    thorium_actor_add_action(self,
                    ACTION_START_REPLY, spate_start_reply);

    /*
     * Register required actor scripts now
     */

    thorium_actor_add_script(self, SCRIPT_INPUT_CONTROLLER,
                    &biosal_input_controller_script);
    thorium_actor_add_script(self, SCRIPT_DNA_KMER_COUNTER_KERNEL,
                    &biosal_dna_kmer_counter_kernel_script);
    thorium_actor_add_script(self, SCRIPT_MANAGER,
                    &core_manager_script);
    thorium_actor_add_script(self, SCRIPT_WRITER_PROCESS,
                    &core_writer_process_script);
    thorium_actor_add_script(self, SCRIPT_AGGREGATOR,
                    &biosal_aggregator_script);
    thorium_actor_add_script(self, SCRIPT_KMER_STORE,
                    &biosal_kmer_store_script);
    thorium_actor_add_script(self, SCRIPT_SEQUENCE_STORE,
                    &biosal_sequence_store_script);
    thorium_actor_add_script(self, SCRIPT_COVERAGE_DISTRIBUTION,
                    &biosal_coverage_distribution_script);
    thorium_actor_add_script(self, SCRIPT_ASSEMBLY_GRAPH_BUILDER,
                    &biosal_assembly_graph_builder_script);

    thorium_actor_add_script(self, SCRIPT_ASSEMBLY_GRAPH_STORE,
                    &biosal_assembly_graph_store_script);
    thorium_actor_add_script(self, SCRIPT_ASSEMBLY_SLIDING_WINDOW,
                    &biosal_assembly_sliding_window_script);
    thorium_actor_add_script(self, SCRIPT_ASSEMBLY_BLOCK_CLASSIFIER,
                    &biosal_assembly_block_classifier_script);
    thorium_actor_add_script(self, SCRIPT_COVERAGE_DISTRIBUTION,
                    &biosal_coverage_distribution_script);

    thorium_actor_add_script(self, SCRIPT_ASSEMBLY_ARC_KERNEL,
                    &biosal_assembly_arc_kernel_script);
    thorium_actor_add_script(self, SCRIPT_ASSEMBLY_ARC_CLASSIFIER,
                    &biosal_assembly_arc_classifier_script);
    thorium_actor_add_script(self, SCRIPT_UNITIG_WALKER,
                    &biosal_unitig_walker_script);
    thorium_actor_add_script(self, SCRIPT_UNITIG_VISITOR,
                    &biosal_unitig_visitor_script);
    thorium_actor_add_script(self, SCRIPT_UNITIG_MANAGER,
                    &biosal_unitig_manager_script);

    concrete_self->block_size = 16 * 4096;

    concrete_self->file_index = 0;
}

void spate_destroy(struct thorium_actor *self)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    core_timer_destroy(&concrete_self->timer);

    concrete_self->input_controller = THORIUM_ACTOR_NOBODY;
    concrete_self->manager_for_sequence_stores = THORIUM_ACTOR_NOBODY;
    concrete_self->assembly_graph = THORIUM_ACTOR_NOBODY;
    concrete_self->assembly_graph_builder = THORIUM_ACTOR_NOBODY;

    core_vector_destroy(&concrete_self->initial_actors);
    core_vector_destroy(&concrete_self->sequence_stores);
    core_vector_destroy(&concrete_self->graph_stores);
}

void spate_receive(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_take_action(self, message);
}

void spate_start(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    int name;
    struct spate *concrete_self;
    int spawner;
    char *directory_name;
    int already_created;
    int argc;
    char **argv;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);
    name = thorium_actor_name(self);

    /*
     * The buffer contains initial actors spawned by Thorium
     */
    core_vector_unpack(&concrete_self->initial_actors, buffer);

    if (!spate_must_print_help(self)) {
        printf("spate/%d starts\n", name);
    }

    if (core_vector_index_of(&concrete_self->initial_actors, &name) == 0) {
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

    /*
     * Otherwise, the coverage distribution will take care of creating
     * the directory.
     */

    core_timer_start(&concrete_self->timer);

    /*
     * Verify if the directory already exists. If it does, don't
     * do anything as it is not a good thing to overwrite previous science
     * results.
     */
    argc = thorium_actor_argc(self);
    argv = thorium_actor_argv(self);
    directory_name = core_command_get_output_directory(argc, argv);
    already_created = core_directory_verify_existence(directory_name);

    if (already_created) {
        printf("%s/%d Error: output directory \"%s\" already exists, please delete it or use a different output directory\n",
                        thorium_actor_script_name(self),
                        thorium_actor_name(self),
                        directory_name);
        spate_stop(self);
        return;
    }


    spawner = thorium_actor_get_spawner(self, &concrete_self->initial_actors);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_INPUT_CONTROLLER);
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

    if (core_vector_index_of(&concrete_self->initial_actors, &source) >= 0) {
        thorium_actor_send_to_self_empty(self, ACTION_STOP);
    }

    if (concrete_self->is_leader) {

        thorium_actor_send_empty(self, concrete_self->assembly_graph_builder,
                        ACTION_ASK_TO_STOP);
        thorium_actor_send_empty(self, concrete_self->manager_for_sequence_stores,
                        ACTION_ASK_TO_STOP);

        if (!spate_must_print_help(self)) {
            core_timer_stop(&concrete_self->timer);
            core_timer_print_with_description(&concrete_self->timer, "Total");
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

        thorium_actor_add_action_with_source(self,
                    ACTION_START_REPLY,
                    spate_start_reply_controller,
                    concrete_self->input_controller);

        printf("spate %d spawned controller %d\n", thorium_actor_name(self),
                        new_actor);

        spawner = thorium_actor_get_spawner(self, &concrete_self->initial_actors);

        thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_MANAGER);

    } else if (concrete_self->manager_for_sequence_stores == THORIUM_ACTOR_NOBODY) {

        concrete_self->manager_for_sequence_stores = new_actor;

        thorium_actor_add_action_with_source(self,
                    ACTION_START_REPLY,
                    spate_start_reply_manager,
                    concrete_self->manager_for_sequence_stores);

        printf("spate %d spawned manager %d\n", thorium_actor_name(self),
                        new_actor);

        spawner = thorium_actor_get_spawner(self, &concrete_self->initial_actors);

        thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_ASSEMBLY_GRAPH_BUILDER);

    } else if (concrete_self->assembly_graph_builder == THORIUM_ACTOR_NOBODY) {

        concrete_self->assembly_graph_builder = new_actor;

        thorium_actor_add_action_with_source(self,
                    ACTION_START_REPLY,
                    spate_start_reply_builder,
                concrete_self->assembly_graph_builder);

        thorium_actor_add_action_with_source(self,
                    ACTION_SET_PRODUCERS_REPLY,
                    spate_set_producers_reply,
                    concrete_self->assembly_graph_builder);

        printf("spate %d spawned graph builder %d\n", thorium_actor_name(self),
                        new_actor);

        thorium_actor_send_int(self, concrete_self->manager_for_sequence_stores, ACTION_MANAGER_SET_SCRIPT,
                        SCRIPT_SEQUENCE_STORE);
    }
}

void spate_set_script_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->initial_actors);
}

void spate_start_reply(struct thorium_actor *self, struct thorium_message *message)
{
}

void spate_start_reply_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct core_vector consumers;
    struct spate *concrete_self;
    void *buffer;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    core_vector_init(&consumers, sizeof(int));

    buffer = thorium_message_buffer(message);

    core_vector_unpack(&consumers, buffer);

    printf("spate %d sends the names of %d consumers to controller %d\n",
                    thorium_actor_name(self),
                    (int)core_vector_size(&consumers),
                    concrete_self->input_controller);

    thorium_actor_send_vector(self, concrete_self->input_controller, ACTION_SET_CONSUMERS, &consumers);

    core_vector_push_back_vector(&concrete_self->sequence_stores, &consumers);

    core_vector_destroy(&consumers);
}

void spate_set_consumers_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    printf("spate %d sends %d spawners to controller %d\n",
                    thorium_actor_name(self),
                    (int)core_vector_size(&concrete_self->initial_actors),
                    concrete_self->input_controller);

    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->initial_actors);
}

void spate_start_reply_controller(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    printf("received reply from controller\n");
    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    /*
     * Stop the actor computation
     */

    thorium_actor_send_reply_int(self, ACTION_SET_BLOCK_SIZE, concrete_self->block_size);
}

void spate_set_block_size_reply(struct thorium_actor *self, struct thorium_message *message)
{

    thorium_actor_send_to_self_empty(self, ACTION_SPATE_ADD_FILES);
}

void spate_add_files(struct thorium_actor *self, struct thorium_message *message)
{
    if (!spate_add_file(self)) {
        thorium_actor_send_to_self_empty(self, ACTION_SPATE_ADD_FILES_REPLY);
    }
}

void spate_add_files_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_actor_send_empty(self, concrete_self->input_controller,
                    ACTION_INPUT_DISTRIBUTE);
}

void spate_distribute_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    printf("spate %d: all sequence stores are ready\n",
                    thorium_actor_name(self));

    thorium_actor_send_vector(self, concrete_self->assembly_graph_builder,
                    ACTION_SET_PRODUCERS, &concrete_self->sequence_stores);

    /* kill the controller
     */

    thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP);
}

void spate_set_producers_reply(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_actor_send_reply_vector(self, ACTION_START, &concrete_self->initial_actors);
}

void spate_start_reply_builder(struct thorium_actor *self, struct thorium_message *message)
{
    void *buffer;
    int spawner;
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);
    buffer = thorium_message_buffer(message);

    core_vector_unpack(&concrete_self->graph_stores, buffer);

    printf("%s/%d has %d graph stores\n",
                    thorium_actor_script_name(self),
                    thorium_actor_name(self),
                    (int)core_vector_size(&concrete_self->graph_stores));

    spawner = thorium_actor_get_spawner(self, &concrete_self->initial_actors);

    concrete_self->unitig_manager = THORIUM_ACTOR_SPAWNING_IN_PROGRESS;

    thorium_actor_add_action_with_condition(self, ACTION_SPAWN_REPLY,
                    spate_spawn_reply_unitig_manager,
                    &concrete_self->unitig_manager, THORIUM_ACTOR_SPAWNING_IN_PROGRESS);

    thorium_actor_send_int(self, spawner, ACTION_SPAWN, SCRIPT_UNITIG_MANAGER);
}

void spate_spawn_reply_unitig_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_message_unpack_int(message, 0, &concrete_self->unitig_manager);

    thorium_actor_add_action_with_source(self, ACTION_START_REPLY,
                    spate_start_reply_unitig_manager,
                    concrete_self->unitig_manager);

    thorium_actor_send_vector(self, concrete_self->unitig_manager,
                    ACTION_START,
                    &concrete_self->initial_actors);
}

void spate_start_reply_unitig_manager(struct thorium_actor *self, struct thorium_message *message)
{
    struct spate *concrete_self;

    concrete_self = (struct spate *)thorium_actor_concrete_actor(self);

    thorium_actor_add_action_with_source(self, ACTION_SET_PRODUCERS_REPLY,
                    spate_set_producers_reply_reply_unitig_manager,
                    concrete_self->unitig_manager);

    thorium_actor_send_reply_vector(self, ACTION_SET_PRODUCERS,
                    &concrete_self->graph_stores);
}

void spate_set_producers_reply_reply_unitig_manager(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_send_reply_empty(self, ACTION_ASK_TO_STOP);

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

        thorium_message_init(&message, ACTION_ADD_FILE, strlen(file) + 1, file);

        thorium_actor_send(self, concrete_self->input_controller, &message);

        thorium_message_destroy(&message);

        ++concrete_self->file_index;

        return 1;
    }

    return 0;
}

void spate_add_file_reply(struct thorium_actor *self, struct thorium_message *message)
{
    thorium_actor_send_to_self_empty(self, ACTION_SPATE_ADD_FILES);
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
    printf("    -k %d (no limit and no recompilation is required)\n", CORE_DEFAULT_KMER_LENGTH);
    printf("    -threads-per-node %d\n", 1);
    printf("    -o %s\n",
                    CORE_DEFAULT_OUTPUT);

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
    thorium_actor_send_range_empty(self, &concrete_self->initial_actors, ACTION_ASK_TO_STOP);
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
