
APPLICATION_BINOMIAL_TREE_PRODUCT=performance/binomial_tree/binomial_tree
APPLICATION_BINOMIAL_TREE_OBJECTS=performance/binomial_tree/main.o performance/binomial_tree/process.o

APPLICATION_EXECUTABLES+=$(APPLICATION_BINOMIAL_TREE_PRODUCT)
APPLICATION_OBJECTS+=$(APPLICATION_BINOMIAL_TREE_OBJECTS)

$(APPLICATION_BINOMIAL_TREE_PRODUCT): $(APPLICATION_BINOMIAL_TREE_OBJECTS) $(LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(CONFIG_LDFLAGS)
