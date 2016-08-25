include cilkMakeF

CODE = callahanKosaraju
COMMON =  utils.h IO.h  sequence.h cilk.h gettime.h 
COMMONB = nnTime.C   PointGenerator.h

all : $(CODE)
	cd ../common; 	make -s nnCheck

$(COMMON) :
	ln -s ../../common/$@ .

$(COMMONB) :
	ln -s ../common/$@ .

$(CODE): $(COMMON) $(COMMONB) CKPointSet.h CKPointSet.cpp utils.h
	$(PCC) $(PCFLAGS) $(PLFLAGS) -include CKPointSet.cpp -o $(CODE) nnTime.C 

clean :
	rm -f $(CODE)

cleansrc :
	make -s clean
	rm -f $(COMMON) $(COMMONB)
