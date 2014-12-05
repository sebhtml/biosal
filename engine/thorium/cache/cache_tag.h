
#ifndef THORIUM_CACHE_TAG_
#define THORIUM_CACHE_TAG_

struct thorium_cache_tag {
    int action;
    int destination;
    int count;
    uint64_t signature;
};

#endif
