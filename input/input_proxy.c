
#include "input_proxy.h"

#include <stdio.h>

/*
#define BSAL_INPUT_PROXY_DEBUG
*/

void bsal_input_proxy_init(struct bsal_input_proxy *proxy,
                char *file)
{
#ifdef BSAL_INPUT_PROXY_DEBUG
    printf("DEBUG bsal_input_proxy_init open file %s\n", file);
#endif

    bsal_input_init(&proxy->input, &proxy->fastq, &bsal_fastq_input_vtable,
                    file);
}

int bsal_input_proxy_get_sequence(struct bsal_input_proxy *proxy,
                struct bsal_dna_sequence *sequence)
{
    return bsal_input_get_sequence(&proxy->input, sequence);
}

void bsal_input_proxy_destroy(struct bsal_input_proxy *proxy)
{
#ifdef BSAL_INPUT_PROXY_DEBUG
    printf("DEBUG bsal_input_proxy_destroy\n");
#endif

    bsal_input_destroy(&proxy->input);
}

int bsal_input_proxy_size(struct bsal_input_proxy *proxy)
{
#ifdef BSAL_INPUT_PROXY_DEBUG
    printf("DEBUG size %i\n", proxy->sequences);
#endif

    return bsal_input_size(&proxy->input);
}
