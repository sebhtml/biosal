
test: tests

TEST_EXECUTABLES=
TEST_OBJECTS=
TEST_RUNS=

# tests

include tests/test_*.mk

tests: $(TEST_RUNS)

