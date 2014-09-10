
GENOMICS_OBJECTS += genomics/assembly/assembly_graph_store.o
GENOMICS_OBJECTS += genomics/assembly/assembly_graph_builder.o
GENOMICS_OBJECTS += genomics/assembly/assembly_sliding_window.o
GENOMICS_OBJECTS += genomics/assembly/assembly_block_classifier.o
GENOMICS_OBJECTS += genomics/assembly/assembly_vertex.o
GENOMICS_OBJECTS += genomics/assembly/assembly_connectivity.o
GENOMICS_OBJECTS += genomics/assembly/assembly_arc.o
GENOMICS_OBJECTS += genomics/assembly/assembly_arc_kernel.o
GENOMICS_OBJECTS += genomics/assembly/assembly_arc_classifier.o
GENOMICS_OBJECTS += genomics/assembly/assembly_arc_block.o
GENOMICS_OBJECTS += genomics/assembly/assembly_graph_summary.o
GENOMICS_OBJECTS += genomics/assembly/vertex_neighborhood.o

include genomics/assembly/unitig/Makefile.mk
