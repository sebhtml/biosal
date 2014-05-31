
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

    proxy->sequences = 0;
}

int bsal_input_proxy_get_sequence(struct bsal_input_proxy *proxy,
                struct bsal_dna_sequence *sequence)
{
    proxy->sequences++;

    if (proxy->sequences < 1234567) {
        return 1;
    }

#ifdef BSAL_INPUT_PROXY_DEBUG
    printf("DEBUG bsal_input_proxy_get_sequence END %i\n",
                    proxy->sequences);
#endif

    return 0;
}

void bsal_input_proxy_destroy(struct bsal_input_proxy *proxy)
{
#ifdef BSAL_INPUT_PROXY_DEBUG
    printf("DEBUG bsal_input_proxy_destroy\n");
#endif

    proxy->sequences = -1;
}

int bsal_input_proxy_size(struct bsal_input_proxy *proxy)
{
#ifdef BSAL_INPUT_PROXY_DEBUG
    printf("DEBUG size %i\n", proxy->sequences);
#endif

    return proxy->sequences;
}
