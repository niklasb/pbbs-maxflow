CC = g++
LFLAGS =
CFLAGS = -O2
#CILK = 1

ifdef OPENMP
PCC = /usr/bin/g++
PCFLAGS = -fopenmp -O2 -DOPENMP
PLFLAGS = -fopenmp

else ifdef GCILK
PCC = g++
PCFLAGS = -lcilkrts -fcilkplus -O2 -DCILKP
PLFLAGS = -lcilkrts -fcilkplus

else ifdef CILK
PCC = cilk++
PCFLAGS = -O2 -DCILK -Wno-cilk-for
PLFLAGS =

else ifdef IPPROOT
PCC = icpc
PCFLAGS = -O2 -DCILKP
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
SEQ = seq.h $(SEQUENCE)
GRAPH = graph.h
INTSORT = intSort.h transpose.h
QOTREE = $(INTSORT) ppOctTree.h
NNGH = nearestNeighbor.h $(QOTREE)
ITEMGEN = itemGen.h gettime.h
STRINGGEN = $(ITEMGEN) stringGen.h
SEQGEN = $(STRINGGEN) seqGen.h
GEOMGEN = $(ITEMGEN) geomGen.h
GRAPHGEN = $(GRAPH) graphGen.h 

sequenceTest: sequenceTest.C $(SEQUENCE) 
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o sequenceTest sequenceTest.C

casTest: casTest.C $(SEQUENCE) 
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o casTest casTest.C

mergeTest: mergeTest.C merge.h $(BASIC)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o mergeTest mergeTest.C 

randPermTest: mergeTest.C randPerm.h $(SEQGEN) $(BASIC)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o randPermTest randPermTest.C 

genSeqRand : genSeqRand.C $(SEQGEN)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o genSeqRand genSeqRand.C

genSeqTrigramString : genSeqTrigramString.C $(SEQGEN)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o genSeqTrigramString genSeqTrigramString.C

genSeqAlmostSorted : genSeqAlmostSorted.C $(SEQGEN)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o genSeqAlmostSorted genSeqAlmostSorted.C

data/seqRand_10M_double : genSeqRand
	./genSeqRand -t double 10000000 data/seqRand_10M_double

genSeqExp : genSeqExp.C $(SEQGEN)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o genSeqExp genSeqExp.C

compSortCheck: compSortCheck.C sampleSort.h $(BASIC) $(SEQGEN)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o compSortCheck compSortCheck.C

quickSortTest: compSortTest.C quickSort.h $(SEQGEN) $(BASIC) randPerm.h
	$(PCC) $(PCFLAGS) $(PLFLAGS) -include quickSort.h -o quickSortTest compSortTest.C

sampleSortTest: compSortTest.C sampleSort.h quickSort.h transpose.h $(SEQGEN) $(BASIC) 
	$(PCC) $(PCFLAGS) $(PLFLAGS) -include sampleSort.h -o sampleSortTest compSortTest.C

serialSortTest: compSortTest.C serialSort.h $(SEQGEN) $(BASIC) 
	$(CC) $(CFLAGS) $(LFLAGS) -include serialSort.h -o serialSortTest compSortTest.C

stlParallelSortTest: compSortTest.C stlParallelSort.h $(SEQGEN) $(BASIC) 
	$(CC) $(GPCFLAGS) $(GPLFLAGS) -include stlParallelSort.h -o stlParallelSortTest compSortTest.C

intSortTest: intSortTest.C $(INTSORT) $(SEQUENCE) $(SEQGEN)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o intSortTest intSortTest.C

remDuplicatesTest: remDuplicatesTest.C hash.h $(SEQGEN) $(SEQ)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o remDuplicatesTest remDuplicatesTest.C

tableTest: tableTest.C hash.h $(SEQGEN) $(SEQ)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o tableTest tableTest.C

graphGen.o: graphGen.C $(INTSORT) hash.h $(GRAPH) $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c graphGen.C

mst.o: mst.C quickSort.h $(GRAPH)
	$(PCC) $(PCFLAGS) -c mst.C

serialMst.o: serialMst.C quickSort.h $(GRAPH)
	$(PCC) $(PCFLAGS) -c serialMst.C

mstTest.o: mstTest.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c mstTest.C

mstTest: graphGen.o mst.o mstTest.o
	$(PCC) $(PLFLAGS) -o mstTest graphGen.o mst.o mstTest.o

serialMstTest: graphGen.o serialMst.o mstTest.o
	$(PCC) $(PLFLAGS) -o serialMstTest graphGen.o serialMst.o mstTest.o

st.o: st.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c st.C

stTest.o: stTest.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c stTest.C

stTest: graphGen.o st.o stTest.o
	$(PCC) $(PLFLAGS) -o stTest graphGen.o st.o stTest.o

serialSt.o: serialSt.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c serialSt.C

serialStTest: graphGen.o serialSt.o stTest.o
	$(PCC) $(PLFLAGS) -o serialStTest graphGen.o serialSt.o stTest.o

ndSt.o: ndSt.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c ndSt.C

ndStTest: graphGen.o ndSt.o stTest.o
	$(PCC) $(PLFLAGS) -o ndStTest graphGen.o ndSt.o stTest.o

maxIndSet.o: maxIndSet.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c maxIndSet.C

maxIndSetTest.o: maxIndSetTest.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c maxIndSetTest.C

maxIndSetTest: maxIndSetTest.o graphGen.o maxIndSet.o
	$(PCC) $(PLFLAGS) -o maxIndSetTest graphGen.o maxIndSet.o maxIndSetTest.o

maxIndSet2.o: maxIndSet2.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c maxIndSet2.C

maxIndSet2Test.o: maxIndSet2Test.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c maxIndSet2Test.C


maxIndSet2Test: maxIndSet2Test.o graphGen.o maxIndSet2.o 
	$(PCC) $(PCFLAGS) -o maxIndSet2Test graphGen.o maxIndSet2.o maxIndSet2Test.o

manis.o: manis.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c manis.C

manisTest.o: manisTest.C
	$(PCC) $(PCFLAGS) -c manisTest.C

manisTest: manisTest.o manis.o 
	$(PCC) $(PLFLAGS) -o manisTest manis.o manisTest.o

ge.o: ge.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c ge.C	

geTest.o: geTest.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c geTest.C

geTest: geTest.o graphGen.o ge.o 
	$(PCC) $(PCFLAGS) -o geTest graphGen.o geTest.o ge.o

serialGe.o: serialGe.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c serialGe.C

serialGeTest: geTest.o graphGen.o serialGe.o 
	$(PCC) $(PCFLAGS) -o serialGeTest graphGen.o geTest.o serialGe.o

colorGraph.o: colorGraph.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c colorGraph.C

colorGraphTest.o: colorGraphTest.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c colorGraphTest.C

colorGraphTest: colorGraphTest.o colorGraph.o graphGen.o maxIndSet.o
	$(PCC) $(PLFLAGS) -o colorGraphTest graphGen.o colorGraph.o maxIndSet.o colorGraphTest.o

maxMatching.o: maxMatching.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c maxMatching.C

maxMatchingTest.o: maxMatchingTest.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c maxMatchingTest.C

maxMatchingTest: graphGen.o maxMatchingTest.o maxMatching.o
	$(PCC) $(PLFLAGS) -o maxMatchingTest graphGen.o maxMatching.o maxMatchingTest.o

maxMatching2.o: maxMatching2.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c maxMatching2.C

maxMatching2Test.o: maxMatching2Test.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c maxMatching2Test.C

maxMatching2Test: graphGen.o maxMatching2Test.o maxMatching2.o
	$(PCC) $(PCFLAGS) -o maxMatching2Test graphGen.o maxMatching2.o maxMatching2Test.o

separator.o: separator.C quickSort.h $(GRAPH)
	$(PCC) $(PCFLAGS) -c separator.C

separatorTest.o: separatorTest.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c separatorTest.C

separatorTest: graphGen.o separatorTest.o separator.o maxMatching.o
	$(PCC) $(PLFLAGS) -o separatorTest graphGen.o separator.o separatorTest.o maxMatching.o

graphGenOmp.o: graphGen.C $(INTSORT) hash.h $(GRAPH) $(GRAPHGEN)
	$(CC) $(CFLAGS) -c graphGen.C -o graphGenOmp.o

bfsOmp.o: bfsOmp.C $(GRAPH)
	$(CC) -fopenmp $(CFLAGS) -c bfsOmp.C

bfsTestOmp.o: bfsTestOmp.C $(GRAPHGEN)
	$(CC) -fopenmp $(CFLAGS) -c bfsTestOmp.C

bfsTestOmp: bfsTestOmp.o graphGenOmp.o bfsOmp.o
	$(CC) -fopenmp $(LFLAGS) -o bfsTestOmp graphGenOmp.o bfsOmp.o bfsTestOmp.o

bfs.o: bfs.C $(GRAPH)
	$(PCC) $(PCFLAGS) -c bfs.C

bfsTest.o: bfsTest.C $(GRAPHGEN)
	$(PCC) $(PCFLAGS) -c bfsTest.C

bfsTest: bfsTest.o graphGen.o bfs.o
	$(PCC) $(PLFLAGS) -o bfsTest graphGen.o bfs.o bfsTest.o

delaunayRefinement.o: delaunayRefinement.C delaunay.h $(NNGH) $(SEQ)
	$(PCC) $(PCFLAGS) -c delaunayRefinement.C 

delaunayRefinementTest.o: delaunayRefinementTest.C geom.h delaunay.h $(GEOMGEN)
	$(PCC) $(PCFLAGS) -c delaunayRefinementTest.C 

delaunayRefinementTest: delaunayRefinement.o delaunayRefinementTest.o
	$(PCC) $(PLFLAGS) -o delaunayRefinementTest delaunayRefinementTest.o delaunayRefinement.o

delaunay.o: delaunay.C delaunay.h $(NNGH) $(SEQ)
	$(PCC) $(PCFLAGS) -c delaunay.C 

delaunayTest.o: delaunayTest.C geom.h delaunay.h $(GEOMGEN)
	$(PCC) $(PCFLAGS) -c delaunayTest.C 

delaunayTest: delaunay.o delaunayTest.o
	$(PCC) $(PLFLAGS) -o delaunayTest delaunayTest.o delaunay.o

ndDelaunay.o: ndDelaunay.C delaunay.h $(NNGH) $(SEQ)
	$(PCC) $(PCFLAGS) -c ndDelaunay.C 

ndDelaunayTest: ndDelaunay.o delaunayTest.o
	$(PCC) $(PLFLAGS) -o ndDelaunayTest delaunayTest.o ndDelaunay.o

serialDelaunay.o: serialDelaunay.C delaunay.h $(NNGH) $(SEQ)
	$(CC) $(CFLAGS) -c serialDelaunay.C 

sdelaunayTest.o: delaunayTest.C geom.h delaunay.h $(GEOMGEN)
	$(CC) $(CFLAGS) -c delaunayTest.C -o sdelaunayTest.o

serialDelaunayTest: serialDelaunay.o sdelaunayTest.o
	$(CC) $(LFLAGS) -o serialDelaunayTest serialDelaunay.o sdelaunayTest.o

hull.o: hull.C geom.h $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c hull.C

hullTest.o: hullTest.C geom.h $(GEOMGEN)
	$(PCC) $(PCFLAGS) -c hullTest.C

hullTest: hull.o hullTest.o
	$(PCC) $(PLFLAGS) -o hullTest hullTest.o hull.o

nearestNeighborTest: nearestNeighborTest.C $(NNGH) $(GEOMGEN) $(SEQ)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o nearestNeighborTest nearestNeighborTest.C

buildTreeTest: buildTreeTest.C $(NNGH) $(GEOMGEN) $(SEQ) 
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o buildTreeTest buildTreeTest.C

barnesHut.o: barnesHut.C geom.h quadOctTree.h $(SEQ) nbody.h
	$(PCC) $(PCFLAGS) -c barnesHut.C

nbodyTest.o: nbodyTest.C geom.h quadOctTree.h $(SEQ) $(GEOMGEN) nbody.h
	$(PCC) $(PCFLAGS) -c nbodyTest.C

nbodyTest: barnesHut.o nbodyTest.o
	$(PCC) $(PLFLAGS) -o nbodyTest barnesHut.o nbodyTest.o

spmvTest.o: spmvTest.C $(GRAPH) spmv.h
	$(PCC) $(PCFLAGS) -c spmvTest.C

spmvTest: spmvTest.o graphGen.o 
	$(PCC) $(PLFLAGS) -o spmvTest spmvTest.o graphGen.o 

editGraph: editGraph.C hash.h $(INTSORT) $(SEQ)
	$(PCC) $(PCFLAGS) $(PLFLAGS) -o editGraph editGraph.C

suffixArrayTest.o: suffixArrayTest.C $(STRINGGEN) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c suffixArrayTest.C

rangeMin.o: rangeMin.C rangeMin.h $(BASIC) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c rangeMin.C

mergeSuffixArrayToTree.o: mergeSuffixArrayToTree.C suffixTree.h cartesianTree.h $(SEQUENCE) $(INTSORT) hash.h 
	$(PCC) $(PCFLAGS) -c mergeSuffixArrayToTree.C

suffixTreeTest.o: suffixTreeTest.C $(STRINGGEN) $(SEQUENCE) hash.h suffixTree.h 
	$(PCC) $(PCFLAGS) -c suffixTreeTest.C

mergeSuffixTreeTest: suffixArray.o rangeMin.o suffixTreeTest.o mergeSuffixArrayToTree.o
	$(PCC) $(PLFLAGS) -o $@ suffixTreeTest.o suffixArray.o rangeMin.o mergeSuffixArrayToTree.o

suffixArray.o: suffixArray.C merge.h $(INTSORT) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c suffixArray.C

suffixArrayTest : suffixArrayTest.o suffixArray.o rangeMin.o
	$(PCC) $(PLFLAGS) -o $@ suffixArrayTest.o suffixArray.o rangeMin.o

rangeMinTest: rangeMinTest.C rangeMin.o
	$(PCC) $(PCFLAGS) rangeMinTest.C $(PLFLAGS) -o rangeMinTest rangeMin.o

suffixArrayTest2.o: suffixArrayTest2.C $(STRINGGEN) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c suffixArrayTest2.C

suffixArray2.o: suffixArray2.C merge.h $(INTSORT) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c suffixArray2.C

suffixArrayTest2 : suffixArrayTest2.o suffixArray2.o
	$(PCC) $(PLFLAGS) -o $@ suffixArrayTest2.o suffixArray2.o


suffixArrayTest3.o: suffixArrayTest3.C $(STRINGGEN) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c suffixArrayTest3.C

suffixArray3.o: suffixArray3.C $(INTSORT) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c suffixArray3.C

suffixArrayTest3 : suffixArrayTest3.o suffixArray3.o
	$(PCC) $(PLFLAGS) -o $@ suffixArrayTest3.o suffixArray3.o

suffixArray3q.o: suffixArray3q.C $(INTSORT) $(SEQUENCE)
	$(PCC) $(PCFLAGS) -c suffixArray3q.C

suffixArrayTest3q : suffixArrayTest3.o suffixArray3q.o
	$(PCC) $(PLFLAGS) -o $@ suffixArrayTest3.o suffixArray3q.o


ALL= sequenceTest mergeTest intSortTest quickSortTest sampleSortTest serialSortTest remDuplicatesTest tableTest maxIndSetTest colorGraphTest maxMatchingTest separatorTest bfsTest delaunayTest nearestNeighborTest spmvTest nbodyTest suffixArrayTest mergeSuffixTreeTest hullTest mstTest serialMstTest randPermTest buildTreeTest suffixArrayTest2 suffixArrayTest3 suffixArrayTest3q maxMatching2Test maxIndSet2Test ndDelaunayTest serialDelaunayTest delaunayRefinementTest stTest serialStTest ndStTest geTest serialGeTest manisTest

all : $(ALL)

clean :
	rm -f *.o $(ALL)
