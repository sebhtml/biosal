
APPLICATION_GC_PRODUCT=applications/gc/gc
APPLICATION_GC_OBJECTS=applications/gc/gc_ratio_calculator.o applications/gc/main.o

APPLICATION_EXECUTABLES+=$(APPLICATION_GC_PRODUCT)
APPLICATION_OBJECTS+=$(APPLICATION_GC_OBJECTS)

$(APPLICATION_GC_PRODUCT): $(APPLICATION_GC_OBJECTS) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
