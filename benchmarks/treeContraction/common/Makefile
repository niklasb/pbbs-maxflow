include parallelDefs

COMMON = IO.h parseCommandLine.h parallel.h runTests.py sequence.h utils.h

%.o : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -c $< -o $@

all: treeContractionCheck generateTree

treeContractionCheck: treeContractionCheck.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o treeContractionCheck treeContractionCheck.C

generateTree: generateTree.C $(COMMON)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o generateTree generateTree.C

$(COMMON) :
	ln -s ../../common/$@ .

clean :
	rm -f treeContractionCheck generateTree *.o

cleansrc :
	make -s clean
	rm -f $(COMMON) *.pyc
