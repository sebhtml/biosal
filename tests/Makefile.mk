
test: tests

TESTS=test_fifo test_fifo_array test_hash_table_group test_hash_table test_node test_vector

# tests
TEST_FIFO=tests/test.o tests/test_fifo.o
TEST_FIFO_ARRAY=tests/test.o tests/test_fifo_array.o
TEST_HASH_TABLE_GROUP=tests/test.o tests/test_hash_table_group.o
TEST_HASH_TABLE=tests/test.o tests/test_hash_table.o
TEST_NODE=tests/test.o tests/test_node.o
TEST_VECTOR=tests/test.o tests/test_vector.o

tests: $(TESTS)
	./test_fifo_array
	./test_fifo
	./test_hash_table_group
	./test_hash_table
	./test_node
	./test_vector


