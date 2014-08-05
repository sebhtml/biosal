
GENOMICS_OBJECTS=

# genomics stuff
GENOMICS_OBJECTS += genomics/helpers/dna_helper.o

GENOMICS_OBJECTS += genomics/kernels/aggregator.o
GENOMICS_OBJECTS += genomics/kernels/dna_kmer_counter_kernel.o

GENOMICS_OBJECTS += genomics/storage/sequence_store.o
GENOMICS_OBJECTS += genomics/storage/partition_command.o
GENOMICS_OBJECTS += genomics/storage/sequence_partitioner.o
GENOMICS_OBJECTS += genomics/storage/kmer_store.o

GENOMICS_OBJECTS += genomics/assembly/assembly_graph_store.o
GENOMICS_OBJECTS += genomics/assembly/assembly_graph.o
GENOMICS_OBJECTS += genomics/assembly/assembly_graph_builder.o
GENOMICS_OBJECTS += genomics/assembly/assembly_sliding_window.o
GENOMICS_OBJECTS += genomics/assembly/assembly_block_classifier.o
GENOMICS_OBJECTS += genomics/assembly/assembly_vertex.o
GENOMICS_OBJECTS += genomics/assembly/assembly_connectivity.o

GENOMICS_OBJECTS += genomics/input/input_stream.o
GENOMICS_OBJECTS += genomics/input/input_proxy.o
GENOMICS_OBJECTS += genomics/input/input.o
GENOMICS_OBJECTS += genomics/input/input_operations.o
GENOMICS_OBJECTS += genomics/input/buffered_reader.o
GENOMICS_OBJECTS += genomics/input/input_controller.o
GENOMICS_OBJECTS += genomics/input/input_command.o
GENOMICS_OBJECTS += genomics/input/mega_block.o

GENOMICS_OBJECTS += genomics/data/dna_sequence.o
GENOMICS_OBJECTS += genomics/data/dna_kmer.o
GENOMICS_OBJECTS += genomics/data/dna_kmer_block.o
GENOMICS_OBJECTS += genomics/data/dna_kmer_frequency_block.o
GENOMICS_OBJECTS += genomics/data/coverage_distribution.o
GENOMICS_OBJECTS += genomics/data/dna_codec.o

GENOMICS_OBJECTS += genomics/formats/fastq_input.o


