
GENOMICS_OBJECTS=

# genomics stuff
GENOMICS_OBJECTS += genomics/helpers/dna_helper.o
GENOMICS_OBJECTS += genomics/helpers/command.o

GENOMICS_OBJECTS += genomics/kernels/aggregator.o
GENOMICS_OBJECTS += genomics/kernels/dna_kmer_counter_kernel.o

GENOMICS_OBJECTS += genomics/storage/sequence_store.o
GENOMICS_OBJECTS += genomics/storage/partition_command.o
GENOMICS_OBJECTS += genomics/storage/sequence_partitioner.o
GENOMICS_OBJECTS += genomics/storage/kmer_store.o

include genomics/assembly/Makefile.mk

GENOMICS_OBJECTS += genomics/input/input_stream.o
GENOMICS_OBJECTS += genomics/input/input_controller.o
GENOMICS_OBJECTS += genomics/input/input_command.o
GENOMICS_OBJECTS += genomics/input/mega_block.o

GENOMICS_OBJECTS += genomics/data/dna_sequence.o
GENOMICS_OBJECTS += genomics/data/dna_kmer.o
GENOMICS_OBJECTS += genomics/data/dna_kmer_block.o
GENOMICS_OBJECTS += genomics/data/dna_kmer_frequency_block.o
GENOMICS_OBJECTS += genomics/data/coverage_distribution.o
GENOMICS_OBJECTS += genomics/data/dna_codec.o

# formats
GENOMICS_OBJECTS += genomics/formats/input_proxy.o
GENOMICS_OBJECTS += genomics/formats/input_format.o
GENOMICS_OBJECTS += genomics/formats/input_format_interface.o
GENOMICS_OBJECTS += genomics/formats/fastq_input.o
GENOMICS_OBJECTS += genomics/formats/fasta_input.o


