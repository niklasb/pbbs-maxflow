include parallelDefs

COMMON = IO.h parseCommandLine.h parallel.h runTests.py sequence.h utils.h

%.o : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -c $< -o $@

randPermCheck: randPermCheck.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o randPermCheck randPermCheck.C

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f randPermCheck *.o

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
