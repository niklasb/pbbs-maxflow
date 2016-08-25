include cilkMakeF

CODE = nnOctTree
COMMON =  utils.h IO.h  sequence.h cilk.h gettime.h 
COMMONB = nnTime.C   PointGenerator.h

all : $(CODE)
	cd ../common; 	make -s nnCheck

$(COMMON) :
	ln -s ../../common/$@ .

$(COMMONB) :
	ln -s ../common/$@ .

$(CODE): $(COMMON) $(COMMONB) ppOctTree.h nearestNeighbor.h nnOctTree.C utils.h
	$(PCC) $(PCFLAGS) $(PLFLAGS) -include nnOctTree.C -o $(CODE) nnTime.C 

clean :
	rm -f $(CODE)

cleansrc :
	make -s clean
	rm -f $(COMMON) $(COMMONB)
