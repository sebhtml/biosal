
test: tests

TEST_LIBRARY_OBJECTS=tests/test.o
TEST_EXECUTABLES=
TEST_OBJECTS=
TEST_RUNS=

# tests

include tests/test_*.mk

test_private: $(TEST_RUNS)

tests: mock_test_target
	$(Q)$(MAKE) -s test_private | tee tests.log
	$(Q)tests/summarize-tests.sh tests.log

mock_test_target:

quality-assurance: qa


qa:
	tests/perform-quality-assurance.sh


