
#include "integer.h"

int core_int_pack_size(int *self)
{
    return sizeof(*self);
}

int core_int_pack(int *self, char *buffer)
{
    core_memory_copy(buffer, self, core_int_pack_size(self));

    return core_int_pack_size(self);
}

int core_int_unpack(int *self, char *buffer)
{
    core_memory_copy(self, buffer, core_int_pack_size(self));

    return core_int_pack_size(self);
}


