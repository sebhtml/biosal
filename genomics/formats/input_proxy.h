
#ifndef BIOSAL_INPUT_PROXY_H
#define BIOSAL_INPUT_PROXY_H

#include <genomics/formats/fastq_input.h>
#include <genomics/formats/fasta_input.h>

/*
 * An input proxy for input files.
 */
struct biosal_input_proxy {
    struct biosal_input_format input;

    struct core_fastq_input fastq;
    struct core_fasta_input fasta;

    int not_found;
    int not_supported;
    int done;
};

void biosal_input_proxy_init(struct biosal_input_proxy *proxy,
                char *file, uint64_t offset, uint64_t maximum_offset);
void biosal_input_proxy_destroy(struct biosal_input_proxy *proxy);

int biosal_input_proxy_get_sequence(struct biosal_input_proxy *proxy,
                char *sequence);
uint64_t biosal_input_proxy_size(struct biosal_input_proxy *proxy);
uint64_t biosal_input_proxy_offset(struct biosal_input_proxy *proxy);
int biosal_input_proxy_error(struct biosal_input_proxy *proxy);

#endif
