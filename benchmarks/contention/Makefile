include parallelDefs

COMMON = IO.h sequenceIO.h parseCommandLine.h parallel.h quickSort.h deterministicHash.h runTests.py sequence.h utils.h gettime.h

all: contention strings experiments

contention: contention.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o contention contention.C

experiments: experiments.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o experiments experiments.C

strings: strings.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o strings strings.C

$(COMMON):
	ln -s ../common/$@

clean: 
	rm contention strings experiments

cleansrc:
	make -s clean
	rm -f $(COMMON) *.pyc