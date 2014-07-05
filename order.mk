
LIBRARY_HOT_CODE += core/structures/fast_ring_hot_code.o
LIBRARY_HOT_CODE += core/structures/ring_queue_hot_code.o
LIBRARY_HOT_CODE += core/engine/node_hot_code.o
LIBRARY_HOT_CODE += core/engine/worker_pool_hot_code.o
LIBRARY_HOT_CODE += core/engine/transport_hot_code.o
LIBRARY_HOT_CODE += core/engine/actor_hot_code.o
LIBRARY_HOT_CODE += core/structures/vector_hot_code.o
LIBRARY_HOT_CODE += core/engine/worker_hot_code.o

LIBRARY_HOT_CODE += core/engine/message.o
LIBRARY_HOT_CODE += core/engine/node.o
LIBRARY_HOT_CODE += core/engine/actor.o
LIBRARY_HOT_CODE += core/engine/script.o
LIBRARY_HOT_CODE += core/engine/worker.o
LIBRARY_HOT_CODE += core/engine/migration.o
LIBRARY_HOT_CODE += core/engine/worker_pool.o
LIBRARY_HOT_CODE += core/engine/active_buffer.o
LIBRARY_HOT_CODE += core/engine/dispatcher.o
LIBRARY_HOT_CODE += core/engine/transport.o

LIBRARY_HOT_CODE += core/patterns/manager.o
LIBRARY_HOT_CODE += genomics/kernels/aggregator.o


LIBRARY_HOT_CODE += core/helpers/actor_helper.o
LIBRARY_HOT_CODE += core/helpers/message_helper.o
LIBRARY_HOT_CODE += core/helpers/vector_helper.o

LIBRARY_HOT_CODE += genomics/helpers/dna_helper.o

LIBRARY_HOT_CODE += core/helpers/map_helper.o
LIBRARY_HOT_CODE += core/helpers/set_helper.o
LIBRARY_HOT_CODE += core/helpers/pair.o

LIBRARY_HOT_CODE += core/helpers/statistics.o

LIBRARY_HOT_CODE += genomics/kernels/dna_kmer_counter_kernel.o

LIBRARY_HOT_CODE += core/system/lock.o
LIBRARY_HOT_CODE += core/system/counter.o
LIBRARY_HOT_CODE += core/system/packer.o

LIBRARY_HOT_CODE += core/system/memory.o
LIBRARY_HOT_CODE += core/system/timer.o
LIBRARY_HOT_CODE += core/system/tracer.o
LIBRARY_HOT_CODE += core/system/ticket_lock.o

LIBRARY_HOT_CODE += core/system/memory_pool.o
LIBRARY_HOT_CODE += core/system/memory_block.o

LIBRARY_HOT_CODE += core/system/atomic.o
LIBRARY_HOT_CODE += core/system/thread.o

LIBRARY_HOT_CODE += genomics/storage/sequence_store.o
LIBRARY_HOT_CODE += genomics/storage/graph_store.o
LIBRARY_HOT_CODE += genomics/storage/partition_command.o
LIBRARY_HOT_CODE += genomics/storage/sequence_partitioner.o
LIBRARY_HOT_CODE += genomics/storage/kmer_store.o


LIBRARY_HOT_CODE += core/structures/hash_table.o
LIBRARY_HOT_CODE += core/structures/hash_table_group.o
LIBRARY_HOT_CODE += core/structures/queue.o
LIBRARY_HOT_CODE += core/structures/vector.o
LIBRARY_HOT_CODE += core/structures/hash_table_group_iterator.o
LIBRARY_HOT_CODE += core/structures/hash_table_iterator.o
LIBRARY_HOT_CODE += core/structures/dynamic_hash_table.o
LIBRARY_HOT_CODE += core/structures/dynamic_hash_table_iterator.o
LIBRARY_HOT_CODE += core/structures/vector_iterator.o
LIBRARY_HOT_CODE += core/structures/map.o
LIBRARY_HOT_CODE += core/structures/map_iterator.o
LIBRARY_HOT_CODE += core/structures/set.o
LIBRARY_HOT_CODE += core/structures/set_iterator.o
LIBRARY_HOT_CODE += core/structures/ring.o
LIBRARY_HOT_CODE += core/structures/linked_ring.o
LIBRARY_HOT_CODE += core/structures/ring_queue.o
LIBRARY_HOT_CODE += core/structures/fast_ring.o



LIBRARY_HOT_CODE += core/hash/murmur_hash_2_64_a.o

LIBRARY_HOT_CODE += genomics/input/input_stream.o
LIBRARY_HOT_CODE += genomics/input/input_proxy.o
LIBRARY_HOT_CODE += genomics/input/input.o
LIBRARY_HOT_CODE += genomics/input/input_operations.o
LIBRARY_HOT_CODE += genomics/input/buffered_reader.o
LIBRARY_HOT_CODE += genomics/input/input_controller.o
LIBRARY_HOT_CODE += genomics/input/input_command.o
LIBRARY_HOT_CODE += genomics/input/mega_block.o

LIBRARY_HOT_CODE += genomics/data/dna_sequence.o
LIBRARY_HOT_CODE += genomics/data/dna_kmer.o
LIBRARY_HOT_CODE += genomics/data/dna_kmer_block.o
LIBRARY_HOT_CODE += genomics/data/coverage_distribution.o
LIBRARY_HOT_CODE += genomics/data/dna_codec.o

LIBRARY_HOT_CODE += genomics/formats/fastq_input.o


