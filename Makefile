##FDPS PATH
#PS_PATH = -I../../fdps/src/
#PS_PATH = -I../fdps/src/
#PS_PATH = -I../../../fdps/src/
#PS_PATH = -I../../fdps/src/
#PS_PATH = -I../../../../project/fdps/src
PS_PATH = -I../FDPS/src
#PS_PATH = -I../fdps.svn/
#PS_PATH  = -I/home/lwang/code/fdps/src

##ARC PATH
ARC_PATH= -I../SDAR/src
#ARC_PATH = -I/home/lwang/code/ARC/include
#ARC_PATH = -I/home/lwang/GitHub/ARModule/src

##Gperftools PATH
#GPERF_PATH = ../../soft/gperftools-2.6.90

ROOT_PATH= .
INCLUDE  = -I${ROOT_PATH}/src

#use_k_computer = yes
#use_xc30_naoj = yes
use_x86 = yes
use_mpi = yes
#debug_mode=yes
#use_intel=yes

ifeq ($(use_k_computer),yes)
CXX = time mpiFCCpx
OPTFLAGS = -Kfast
OPTFLAGS += -Nfjcex
OPTFLAGS += -Krestp=all
OPTFLAGS += -Ntl_trt
#OPTFLAGS += -NRtrap

CXXFLAGS = -Kopenmp
CXXFLAGS += -x32
CXXFLAGS += -Xg
CXXFLAGS += -std=c++11
CXXFLAGS += -DINTRINSIC_K

FDPSFLAGS = -DPARTICLE_SIMULATOR_THREAD_PARALLEL
FDPSFLAGS += -DPARTICLE_SIMULATOR_MPI_PARALLEL
FDPSFLAGS += -DFAST_ALL_TO_ALL_FOR_K
FDPSFLAGS += -DFAST_WALK_K
# profiling
endif

ifeq ($(use_xc30_naoj),yes)
CXX = time CC
CXXNOMPI = time CC
OPTFLAGS = -O3 -Wall

CXXFLAGS = -ffast-math -funroll-loops
CXXFLAGS += -march=core-avx2
CXXFLAGS += -std=c++11
CXXFLAGS += -DINTRINSIC_X86

#FDPSFLAGS += -DUSE_GNU_PARALLEL_SORT
endif

ifeq ($(use_x86),yes)
ifeq ($(use_mpi),yes)
CXX = time mpicxx
ifeq ($(use_intel),yes)
CXXNOMPI = time icc
else
CXXNOMPI = time g++
endif
#CXX = kinst-ompp mpicxx
#CXX = tau_cxx.sh  -tau_makefile=/opt/tau-2.26.3/x86_64/lib/Makefile.tau-mpi-openmp -tau_options=-optCompInst
FDPSFLAGS = -DPARTICLE_SIMULATOR_MPI_PARALLEL
FDPSFLAGS += -DMPICH_IGNORE_CXX_SEEKC
else
ifeq ($(use_intel),yes)
CXX = time icc
CXXNOMPI = time icc
else
CXX = time g++
CXXNOMPI = time g++
endif
endif

ifeq ($(debug_mode),yes)
OPTFLAGS = -g -O0 -fbounds-check -Wall -D SANITY_CHECK_REALLOCATABLE_ARRAY 
else
OPTFLAGS = -O2 -Wall 
#OPTFLAGS += -ffast-math -funroll-loops
endif

CXXFLAGS = -std=c++11
CXXFLAGS += -Wall
CXXFLAGS += -fopenmp
#CXXFLAGS += -march=skylake-avx512
CXXFLAGS += -march=core-avx2
#CXXFLAGS += -mavx
CXXFLAGS += -DINTRINSIC_X86

FDPSFLAGS += -DPARTICLE_SIMULATOR_THREAD_PARALLEL
#FDPSFLAGS += -DPARTICLE_SIMULATOR_DEBUG_PRINT
#FDPSFLAGS += -DUSE_GNU_PARALLEL_SORT
endif

CXXFLAGS += -DDIV_FIX
#CXXFLAGS += -DP3T_64BIT
CXXFLAGS += -DUSE_QUAD
CXXFLAGS += -DUSE_SIMD
CXXFLAGS += -D PROFILE
CXXFLAGS += -D HARD_CHECK_ENERGY
#CXXFLAGS += ${shell gsl-config --cflags}
#CXXLIBS += ${shell gsl-config --libs}

#MT_FLAGS += -D HARD_CM_KICK
MT_FLAGS += -D SOFT_PERT
MT_FLAGS += -D AR_TTL
MT_FLAGS += -D AR_SLOWDOWN_INNER
#MT_FLAGS += -D AR_TTL_GT_BINARY_INNER
#MT_FLAGS += -D FIX_CHANGEOVER
MT_FLAGS += -D AR_SLOWDOWN_TIMESCALE
#MT_FLAGS += -D AR_SLOWDOWN_MASSRATIO
MT_FLAGS += -D CLUSTER_VELOCITY
#MT_FLAGS += -D KDKDK_2ND
#MT_FLAGS += -D KDKDK_4TH
#MT_FLAGS += -D ONLY_SOFT

SIMD_DEBFLAGS += -DCALC_EP_64bit
SIMD_DEBFLAGS += -DRSQRT_NR_EPJ_X2
SIMD_DEBFLAGS += -DRSQRT_NR_EPJ_X4
SIMD_DEBFLAGS += -DCALC_SP_64bit
SIMD_DEBFLAGS += -DRSQRT_NR_SPJ_X2
SIMD_DEBFLAGS += -DRSQRT_NR_SPJ_X4
SIMD_DEBFLAGS += -DAVX_PRELOAD

#DEBFLAGS += -D INTEGRATED_CUTOFF_FUNCTION
DEBFLAGS += -D PETAR_DEBUG
DEBFLAGS += -D AR_DEBUG
DEBFLAGS += -D AR_DEBUG_DUMP
#DEBFLAGS += -D AR_DEBUG_PRINT
#DEBFLAGS += -D AR_DEEP_DEBUG
DEBFLAGS += -D AR_WARN
DEBFLAGS += -D HARD_DEBUG
DEBFLAGS += -D HARD_DUMP
#DEBFLAGS += -D STABLE_CHECK_DEBUG
DEBFLAGS += -D CLUSTER_DEBUG
DEBFLAGS += -D ARTIFICIAL_PARTICLE_DEBUG
#DEBFLAGS += -D CLUSTER_DEBUG_PRINT
#DEBFLAGS += -D HARD_CLUSTER_PRINT
#DEBFLAGS += -D HARD_DEBUG_PRINT
#DEBFLAGS += -D HARD_DEBUG_PROFILE
#DEBFLAGS += -D NAN_CHECK_DEBUG
#DEBFLAGS += -D DATA_DEBUG
#DEBFLAGS += -D FIX_STEP_DEBUG
#DEBFLAGS += -D DEBUG
#DEBFLAGS += -D MAIN_DEBUG
#DEBFLAGS += -D CORRECT_FORCE_DEBUG

DEBUG_OPT_FLAGS = -g -O0 -fbounds-check -Wall  -D SANITY_CHECK_REALLOCATABLE_ARRAY
HARD_DEBFLAGS+= -D AR_DEBUG -D AR_DEBUG_DUMP -D AR_WARN -D HARD_DEBUG -D HARD_CHECK_ENERGY -D HARD_DEBUG_PRINT -D ADJUST_GROUP_DEBUG -D HERMITE_DEBUG
HARD_MT_FLAGS += -D AR_TTL -D AR_SLOWDOWN_INNER -D AR_SLOWDOWN_TIMESCALE -D STABLE_CHECK_DEBUG_PRINT -D ARTIFICIAL_PARTICLE_DEBUG

ifneq (x$(GPERF_PATH),x)
CXXLIBS += -L$(GPERF_PATH)/lib -lprofiler -ltcmalloc 
endif

VPATH=${ROOT_PATH}/src ${ROOT_PATH}/test

SRC = main.cc ${shell ls ${ROOT_PATH}/src/*.hpp} 

all: nbody.out

nbody.out: $(SRC)
	$(CXX) $(PS_PATH) $(ARC_PATH) $(INCLUDE) $(OPTFLAGS) $(CXXFLAGS) $(FDPSFLAGS) $(MT_FLAGS) $(DEBFLAGS) -o $@ $< $(CXXLIBS)

hard_debug.out: hard_debug.cxx
	$(CXXNOMPI) $(PS_PATH) $(ARC_PATH) $(INCLUDE) $(DEBUG_OPT_FLAGS) $(CXXFLAGS) $(MT_FLAGS) $(HARD_DEBFLAGS) -D HARD_DEBUG_PRINT_TITLE -D STABLE_CHECK_DEBUG -o $@ $< $(CXXLIBS)

hard_test.out: hard_test.cxx
	$(CXXNOMPI) $(PS_PATH) $(ARC_PATH) $(INCLUDE) $(DEBUG_OPT_FLAGS) $(CXXFLAGS) $(HARD_MT_FLAGS) $(HARD_DEBFLAGS) -o $@ $< $(CXXLIBS)

simd_test.out: simd_test.cxx
	$(CXXNOMPI) $(PS_PATH) $(ARC_PATH) $(INCLUDE) $(OPTFLAGS) $(CXXFLAGS) $(SIMD_DEBFLAGS)  $< -o $@  $(CXXLIBS)

tidal_tensor_test.out: tidal_tensor_test.cxx
	$(CXXNOMPI) $(PS_PATH) $(ARC_PATH) $(INCLUDE) $(DEBUG_OPT_FLAGS) $(CXXFLAGS)  $< -o $@  $(CXXLIBS)

clean:
	rm *.out *.o 
