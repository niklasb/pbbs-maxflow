include parallelDefs

COMMON = IO.h parseCommandLine.h parallel.h runTests.py sequence.h utils.h graph.h graphUtils.h blockRadixSort.h deterministicHash.h transpose.h graphIO.h merge.h

%.o : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -c $< -o $@

CHECKFILES = MVGraphCheck.o

MVGraphCheck : $(CHECKFILES)
	$(PCC) $(PLFLAGS) -o $@ $(CHECKFILES)

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f MVGraphCheck *.o

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
