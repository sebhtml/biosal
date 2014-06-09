
test: tests

TESTS=test_queue test_queue_group test_hash_table_group test_hash_table test_node test_vector test_dynamic_hash_table

# tests
TEST_FIFO=tests/test.o tests/test_queue.o
TEST_FIFO_ARRAY=tests/test.o tests/test_queue_group.o
TEST_HASH_TABLE_GROUP=tests/test.o tests/test_hash_table_group.o
TEST_HASH_TABLE=tests/test.o tests/test_hash_table.o
TEST_DYNAMIC_HASH_TABLE=tests/test.o tests/test_dynamic_hash_table.o
TEST_NODE=tests/test.o tests/test_node.o
TEST_VECTOR=tests/test.o tests/test_vector.o

tests: $(TESTS)
	./test_queue_group
	./test_queue
	./test_hash_table_group
	./test_hash_table
	./test_node
	./test_vector
	./test_dynamic_hash_table
