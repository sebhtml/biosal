
#ifndef CORE_WRITER_PROCESS_H
#define CORE_WRITER_PROCESS_H

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
struct core_writer_process {
    int has_file;
    struct core_buffered_file_writer writer;
};

extern struct thorium_script core_writer_process_script;

#endif
