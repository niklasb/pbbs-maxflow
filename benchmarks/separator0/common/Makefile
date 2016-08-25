include parallelDefs

COMMON = IO.h parseCommandLine.h parallel.h runTests.py sequence.h utils.h graph.h graphUtils.h blockRadixSort.h deterministicHash.h transpose.h graphIO.h quickSort.h

%.o : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -c $< -o $@

CHECKFILES = SeparatorCheck.o 

GRAPHFILES = SepToAdjGraph.o

SeparatorCheck : $(CHECKFILES)
	$(PCC) $(PLFLAGS) -o $@ $(CHECKFILES)

SepToAdjGraph : $(GRAPHFILES)
	$(PCC) $(PLFLAGS) -o $@ $(GRAPHFILES)

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f SeparatorCheck SepToAdjGraph *.o

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
