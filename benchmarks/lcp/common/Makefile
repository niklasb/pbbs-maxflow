include parallelDefs

COMMON = IO.h parseCommandLine.h parallel.h runTests.py sequence.h utils.h

LCPCheck: LCPCheck.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o LCPCheck LCPCheck.C

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f LCPCheck

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
