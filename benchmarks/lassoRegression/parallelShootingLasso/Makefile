include cilkMakeF

CODE = lassoShooting
COMMON =  utils.h IO.h  sequence.h cilk.h gettime.h 
COMMONB = lassoTime.C loadLassoData.h 

all : $(CODE)
	cd ../common; 	make -s lassoCheck

$(COMMON) :
	ln -s ../../common/$@ .

$(COMMONB) :
	ln -s ../common/$@ .

$(CODE): $(COMMON) $(COMMONB) lassoShooting.h cas_array.h utils.h
	$(PCC) $(PCFLAGS) $(PLFLAGS) -include lassoShooting.h -o $(CODE) lassoTime.C 

clean :
	rm -f $(CODE)

cleansrc :
	make -s clean
	rm -f $(COMMON) $(COMMONB)
