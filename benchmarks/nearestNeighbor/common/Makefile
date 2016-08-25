include cilkMakeF

COMMON = IO.h cilk.h runTests.py sequence.h utils.h 

nnCheck : nnCheck.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o nnCheck nnCheck.C

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f nnCheck

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
