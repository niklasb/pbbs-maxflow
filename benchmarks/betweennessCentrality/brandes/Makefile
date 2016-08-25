include parallelDefs

# required files
GLOBAL_REQUIRE = utils.h parallel.h sequence.h graph.h blockRadixSort.h transpose.h
LOCAL_REQUIRE =
OBJS = BC.o BFS.o

include ../common/timeRequiredFiles
include MakeBench

BFS.o : BFS.C BFS.h
	$(PCC) $(PCFLAGS) -c $< -o $@

