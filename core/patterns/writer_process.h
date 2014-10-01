
#ifndef BIOSAL_WRITER_PROCESS_H
#define BIOSAL_WRITER_PROCESS_H

#include <core/file_storage/output/buffered_file_writer.h>

#include <engine/thorium/actor.h>

#define SCRIPT_WRITER_PROCESS 0xfe411407

/*
 * Actions supported by the script SCRIPT_WRITER_PROCESS.
 */
#define ACTION_OPEN         0x00219dfd
#define ACTION_OPEN_REPLY   0x0014a682
#define ACTION_WRITE        0x002d7297
#define ACTION_WRITE_REPLY  0x003c5e30
#define ACTION_CLOSE        0x0009a700
#define ACTION_CLOSE_REPLY  0x0021a82c

/*
 * This actor is simple. It can open a file,
 * write to a file, and close a file.
 *
 * This is it.
 */
struct biosal_writer_process {
    int has_file;
    struct biosal_buffered_file_writer writer;
};

extern struct thorium_script biosal_writer_process_script;

void biosal_writer_process_init(struct thorium_actor *self);
void biosal_writer_process_destroy(struct thorium_actor *self);
void biosal_writer_process_receive(struct thorium_actor *self, struct thorium_message *message);

#endif
