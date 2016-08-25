ifdef LONG
INTT = -DLONG
endif

CC = g++
LFLAGS =
CFLAGS = -O2 $(INTT)

ifdef OPENMP
PCC = /usr/bin/g++
PCFLAGS = -fopenmp -O2 -DOPENMP $(INTT)
PLFLAGS = -fopenmp

else ifdef CILK
PCC = cilk++
PCFLAGS = -O2 -DCILK -Wno-cilk-for $(INTT)
PLFLAGS =

else ifdef IPPROOT
PCC = icpc
PCFLAGS = -O2 -DCILKP $(INTT)
PLFLAGS =
CC = icpc
LFLAGS =
CFLAGS = -O2

else 
PCC = $(CC)
PLFLAGS = $(LFLAGS)
PCFLAGS = $(CFLAGS)
endif

BASIC = cilk.h utils.h
SEQUENCE = sequence.h $(BASIC)
INTSORT = intSort.h transpose.h
STRINGGEN = stringGen.h

ALL= suffixTree suffixArray

all : $(ALL)

clean :
	rm -f *.o $(ALL)

suffixArrayTest.o: suffixArrayTest.C $(STRINGGEN) $(SEQUENCE) stackSpace.h
	$(PCC) $(PCFLAGS) -c suffixArrayTest.C

rangeMin.o: rangeMin.C rangeMin.h $(BASIC) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c rangeMin.C

mergeSuffixArrayToTree.o: mergeSuffixArrayToTree.C suffixTree.h cartesianTree.h $(SEQUENCE) $(INTSORT) hash.h hash2.h stackSpace.h
	$(PCC) $(PCFLAGS) -c mergeSuffixArrayToTree.C

suffixTreeTest.o: suffixTreeTest.C $(STRINGGEN) $(SEQUENCE) hash.h suffixTree.h stackSpace.h
	$(PCC) $(PCFLAGS) -c suffixTreeTest.C

suffixTree: suffixArray.o rangeMin.o suffixTreeTest.o mergeSuffixArrayToTree.o 
	$(PCC) $(PLFLAGS) -o $@ suffixTreeTest.o suffixArray.o rangeMin.o mergeSuffixArrayToTree.o 

suffixArray.o: suffixArray.C merge.h $(INTSORT) $(SEQUENCE) stackSpace.h
	$(PCC) $(PCFLAGS) -c suffixArray.C

suffixArray : suffixArrayTest.o suffixArray.o rangeMin.o
	$(PCC) $(PLFLAGS) -o $@ suffixArrayTest.o suffixArray.o rangeMin.o