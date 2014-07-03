
GENOMICS =

# kernels for genomics
include genomics/kernels/Makefile.mk
GENOMICS += $(KERNELS)

# storage
include genomics/storage/Makefile.mk
GENOMICS += $(STORAGE)

include genomics/helpers/Makefile.mk
GENOMICS += $(HELPERS)

# inputs for actors
include genomics/input/Makefile.mk
GENOMICS += $(INPUT)

# data storage
include genomics/data/Makefile.mk
GENOMICS += $(DATA)

# formats
include genomics/formats/Makefile.mk
GENOMICS += $(FORMATS)


