rm deterministicHash.h
ln -s ../benchmarks/common/deterministicHash.h deterministicHash.h;
make clean; make;
unset CILK_NWORKERS
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c

export CILK_NWORKERS=1
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c

rm deterministicHash.h; 
ln -s ../benchmarks/common/ndHash.h deterministicHash.h;
make clean; make;
unset CILK_NWORKERS
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c

export CILK_NWORKERS=1
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c

rm deterministicHash.h; 
ln -s ../benchmarks/removeDuplicates/cuckooHash/cuckooHash.h deterministicHash.h;
make clean; make;
unset CILK_NWORKERS
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c

export CILK_NWORKERS=1
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c
