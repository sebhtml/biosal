
test: tests

TEST_EXECUTABLES=
TEST_OBJECTS=
TEST_RUNS=

# tests

include tests/test_*.mk

test_private: $(TEST_RUNS)

tests: mock_test_target
	$(Q)$(MAKE) test_private | tee tests.log
	tests/summarize-tests.sh tests.log

mock_test_target:
