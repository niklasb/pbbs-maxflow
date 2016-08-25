include parallelDefs

COMMON = IO.h parseCommandLine.h parallel.h runTests.py sequence.h utils.h

%.o : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -c $< -o $@

listRankingCheck: listRankingCheck.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o listRankingCheck listRankingCheck.C

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f listRankingCheck *.o

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
