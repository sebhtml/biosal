
test: tests

TESTS=tests/test_queue tests/test_queue_group tests/test_hash_table_group tests/test_hash_table tests/test_node tests/test_vector tests/test_dynamic_hash_table \
	tests/test_packer tests/test_dna_sequence tests/test_map tests/test_set

# tests
TEST_FIFO=tests/test.o tests/test_queue.o
TEST_FIFO_ARRAY=tests/test.o tests/test_queue_group.o
TEST_HASH_TABLE_GROUP=tests/test.o tests/test_hash_table_group.o
TEST_HASH_TABLE=tests/test.o tests/test_hash_table.o
TEST_DYNAMIC_HASH_TABLE=tests/test.o tests/test_dynamic_hash_table.o
TEST_MAP=tests/test.o tests/test_map.o
TEST_SET=tests/test.o tests/test_set.o
TEST_NODE=tests/test.o tests/test_node.o
TEST_VECTOR=tests/test.o tests/test_vector.o
TEST_PACKER=tests/test.o tests/test_packer.o
TEST_DNA_SEQUENCE=tests/test.o tests/test_dna_sequence.o

# tests

tests/test_vector: $(LIBRARY) $(TEST_VECTOR)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_dna_sequence: $(LIBRARY) $(TEST_DNA_SEQUENCE)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_packer: $(LIBRARY) $(TEST_PACKER)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_node: $(LIBRARY) $(TEST_NODE)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_queue: $(LIBRARY) $(TEST_FIFO)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_hash_table: $(LIBRARY) $(TEST_HASH_TABLE)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_hash_table_group: $(LIBRARY) $(TEST_HASH_TABLE_GROUP)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_queue_group: $(LIBRARY) $(TEST_FIFO_ARRAY)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_dynamic_hash_table: $(LIBRARY) $(TEST_DYNAMIC_HASH_TABLE)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_map: $(LIBRARY) $(TEST_MAP)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests/test_set: $(LIBRARY) $(TEST_SET)
	$(Q)$(ECHO) "  LD $@"
	$(Q)$(CC) $(CFLAGS) $^ -o $@

tests: $(TESTS)
	tests/test_queue_group
	tests/test_queue
	tests/test_hash_table_group
	tests/test_hash_table
	tests/test_node
	tests/test_vector
	tests/test_dynamic_hash_table
	tests/test_map
	tests/test_set
	tests/test_packer
	tests/test_dna_sequence
