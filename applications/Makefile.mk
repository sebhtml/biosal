
APPLICATION_ARGONNITE_PRODUCT=applications/argonnite
APPLICATION_ARGONNITE_OBJECTS=applications/argonnite_kmer_counter/argonnite.o applications/argonnite_kmer_counter/main.o

# applications
$(APPLICATION_ARGONNITE_PRODUCT): $(APPLICATION_ARGONNITE_OBJECTS) $(LIBRARY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

APPLICATIONS= $(APPLICATION_ARGONNITE_PRODUCT)

