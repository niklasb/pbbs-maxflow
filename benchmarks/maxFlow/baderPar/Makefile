include parallelDefs

# required files
GLOBAL_REQUIRE = utils.h parallel.h sequence.h graph.h blockRadixSort.h transpose.h
LOCAL_REQUIRE = 
OBJS = maxFlow.o
PCFLAGS := $(PCFLAGS) -std=c++11 -Wall -g
PLFLAGS := $(PLFLAGS) -ltbb

include ../common/timeRequiredFiles
include MakeBench
