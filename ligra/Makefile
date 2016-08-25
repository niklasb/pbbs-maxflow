ifdef LONG
INTT = -DLONG
endif

ifdef EDGELONG
INTE = -DEDGELONG
endif

# no compare and swap!
ifdef OPENMP
PCC = g++
PCFLAGS = -fopenmp -O3 -DOPENMP $(INTT) $(INTE)

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -O3 -DCILKP $(INTT) $(INTE)

else
PCC = g++
PCFLAGS = -O2 $(INTT) $(INTE)
endif

COMMON= ligra.h graph.h utils.h IO.h parallel.h gettime.h quickSort.h

ALL= BFS BC Components Radii PageRank PageRankDelta BellmanFord

all: $(ALL)

% : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -o $@ $< 

.PHONY : clean

clean :
	rm -f *.o $(ALL)