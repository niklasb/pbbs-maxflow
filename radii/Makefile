include parallelDefs
BNCHMRK = Diameter
# required files
GLOBAL_REQUIRE = utils.h parallel.h sequence.h graph.h blockRadixSort.h transpose.h IO.h
LOCAL_REQUIRE = Diameter.h BFS.h ST.h SPMV.h
OBJS = Diameter.o
CHECK = $(BNCHMRK)Check
TIME = $(BNCHMRK)Time

#include ../common/timeRequiredFiles
#include MakeBench

all : $(BNCHMRK) sparse2bin edgeArray2bin

# Make all implementation objects
%.o : %.C $(GLOBAL_REQUIRE) $(LOCAL_REQUIRE)
	$(PCC) $(PCFLAGS) -c $< -o $@

# Make timing code object file
$(TIME).o : $(TIME).C $(GLOBAL_REQUIRE) $(LOCAL_REQUIRE)
	$(PCC) $(PCFLAGS) -c $< -o $@

# Make benchmark
$(BNCHMRK) : $(TIME).o $(OBJS)
	$(PCC) $(PLFLAGS) -o $@ $(TIME).o $(OBJS)


sparse2bin : sparse2bin.C $(GLOBAL_REQUIRE)
	$(PCC) $(PCFLAGS) sparse2bin.C -o $@ 

edgeArray2bin : edgeArray2bin.C $(GLOBAL_REQUIRE)
	$(PCC) $(PCFLAGS) edgeArray2bin.C -o $@ 

clean :
	rm -f Diameter *.o
cleanall :
	rm -f Diameter sparse2bin edgeArray2bin *.o