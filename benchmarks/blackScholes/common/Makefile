include parallelDefs

COMMON = runTests.py

blackScholesCheck: blackScholesCheck.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o blackScholesCheck blackScholesCheck.C

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f blackScholesCheck

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc