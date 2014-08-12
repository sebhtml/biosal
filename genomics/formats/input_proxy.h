
#ifndef BSAL_INPUT_PROXY_H
#define BSAL_INPUT_PROXY_H

#include <genomics/formats/fastq_input.h>
#include <genomics/formats/fasta_input.h>

/*
 * An input proxy for input files.
 */
struct bsal_input_proxy {
    struct bsal_input_format input;

    struct bsal_fastq_input fastq;
    struct bsal_fasta_input fasta;

    int not_found;
    int not_supported;
    int done;
};

void bsal_input_proxy_init(struct bsal_input_proxy *proxy,
                char *file, uint64_t offset, uint64_t maximum_offset);
int bsal_input_proxy_get_sequence(struct bsal_input_proxy *proxy,
                char *sequence);
void bsal_input_proxy_destroy(struct bsal_input_proxy *proxy);
uint64_t bsal_input_proxy_size(struct bsal_input_proxy *proxy);
uint64_t bsal_input_proxy_offset(struct bsal_input_proxy *proxy);
int bsal_input_proxy_error(struct bsal_input_proxy *proxy);

void bsal_input_proxy_try(struct bsal_input_proxy *proxy,
                struct bsal_input_format *input, void *implementation,
                struct bsal_input_format_interface *operations, char *file,
                uint64_t offset, uint64_t maximum_offset);

#endif
