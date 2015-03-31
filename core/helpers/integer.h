
#ifndef INTEGER_HEADER
#define INTEGER_HEADER


int core_int_pack_size(int *self);
int core_int_pack(int *self, char *buffer);
int core_int_unpack(int *self, char *buffer);

#endif /* INTEGER_HEADER */
