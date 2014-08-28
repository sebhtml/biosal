
APPLICATION_SPATE_PRODUCT=applications/spate_metagenome_assembler/spate
APPLICATION_SPATE_OBJECTS=applications/spate_metagenome_assembler/spate.o applications/spate_metagenome_assembler/main.o

APPLICATION_EXECUTABLES+=$(APPLICATION_SPATE_PRODUCT)
APPLICATION_OBJECTS+=$(APPLICATION_SPATE_OBJECTS)

$(APPLICATION_SPATE_PRODUCT): $(APPLICATION_SPATE_OBJECTS) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

