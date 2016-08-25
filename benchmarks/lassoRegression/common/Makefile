include parallelDefs

COMMON = parallel.h parseCommandLine.h

lassoCheck : lassoCheck.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o lassoCheck lassoCheck.C

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f lassoCheck

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
