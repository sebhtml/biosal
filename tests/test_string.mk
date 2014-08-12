TEST_STRING_NAME=string
TEST_STRING_EXECUTABLE=tests/test_$(TEST_STRING_NAME)
TEST_STRING_OBJECTS=tests/test_$(TEST_STRING_NAME).o
TEST_EXECUTABLES+=$(TEST_STRING_EXECUTABLE)
TEST_OBJECTS+=$(TEST_STRING_OBJECTS)
$(TEST_STRING_EXECUTABLE): $(LIBRARY_OBJECTS) $(TEST_STRING_OBJECTS) $(TEST_LIBRARY_OBJECTS)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
TEST_STRING_RUN=test_run_$(TEST_STRING_NAME)
$(TEST_STRING_RUN): $(TEST_STRING_EXECUTABLE)
	./$^
TEST_RUNS+=$(TEST_STRING_RUN)
