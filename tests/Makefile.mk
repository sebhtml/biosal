
test: tests

# Targets for quality assurance:
#
# - unit-tests			(synonym: tests)
#   Run unit tests in tests/
#
# - example-tests 		(synonym: examples)
#   Run examples in examples/
#
# - performance-tests
#   Run performance tests in performance/
#
# - application-tests
#   Run applications in applications/
#
# - all-tests				(synonym: qa, this runs all tests)
#
# Test results are generated in standard output.
# Also, these files are generated:
#
# - unit-tests.junit.xml
# - example-tests.junit.xml
# - performance-tests.junit.xml
# - application-tests.junit.xml
#
# The XSD file is
# https://svn.jenkins-ci.org/trunk/hudson/dtkit/dtkit-format/dtkit-junit-model/src/main/resources/com/thalesgroup/dtkit/junit/model/xsd/junit-4.xsd

TEST_LIBRARY_OBJECTS=tests/test.o
TEST_EXECUTABLES=
TEST_OBJECTS=
TEST_RUNS=

# tests

include tests/test_*.mk

test_private: $(TEST_RUNS)

tests: mock_test_target
	$(Q)tests/run-unit-tests.sh

mock_test_target:

quality-assurance: qa


qa: all-tests

all-tests:
	tests/perform-quality-assurance.sh

unit-tests: tests

example-tests: examples

application-tests:
	tests/run-application-tests.sh

examples: mock_examples
	tests/run-examples.sh

mock_examples:


