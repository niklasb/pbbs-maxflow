include parallelDefs

COMMON = IO.h parseCommandLine.h parallel.h runTests.py sequence.h utils.h

localAlignmentCheck: localAlignmentCheck.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o localAlignmentCheck localAlignmentCheck.C

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f localAlignmentCheck

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
