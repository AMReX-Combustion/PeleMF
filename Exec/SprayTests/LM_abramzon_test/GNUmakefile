#
# Build configuration
#

DIM             = 2
COMP            = llvm
DEBUG           = FALSE
USE_MPI         = FALSE
USE_OMP         = FALSE
PRECISION       = DOUBLE

#BL_NO_FORT = TRUE
#WARN_ALL = TRUE

USE_CUDA        = FALSE

TINY_PROFILE = FALSE
#USE_WBAR = TRUE

USE_EB = FALSE

USE_SUNDIALS_PP = TRUE
# ifeq ($(USE_SUNDIALS_PP), TRUE)
#   # use KLU sparse features -- only useful if CVODE is used on CPU
#   USE_KLU_PP = FALSE
#   USE_CUDA_SUNDIALS_PP=FALSE
# endif

# Enable ht.velocity_plotfile option
#USE_VELOCITY    = FALSE

Chemistry_Model := decane_3sp

# This sets the EOS directory in $(PELE_PHYSICS_HOME)/Eos
Eos_dir     := Fuego

# This sets the network directory in $(PELE_PHYSICS_HOME)/Reactions
Reactions_dir := Fuego

# This sets the transport directory in $(PELE_PHYSICS_HOME)/Transport
Transport_dir := Simple

USE_PARTICLES = TRUE
SPRAY_FUEL_NUM = 1
CEXE_sources += SprayParticlesInitInsert.cpp

Blocs   := .

include $(PELELM_HOME)/Tools/Make/Make.PeleLM
