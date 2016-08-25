rm deterministicHash.h
ln -s ../benchmarks/removeDuplicates/chainedHash80/chainedHash.h deterministicHash.h;
make clean; make;
unset CILK_NWORKERS
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c

rm deterministicHash.h
ln -s ../benchmarks/removeDuplicates/chainedHash/chainedHash.h deterministicHash.h;
make clean; make;
export CILK_NWORKERS=1
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c

rm deterministicHash.h
ln -s ../benchmarks/removeDuplicates/chainedHashOpt80/chainedHash.h deterministicHash.h;
make clean; make;
unset CILK_NWORKERS
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c

rm deterministicHash.h
ln -s ../benchmarks/removeDuplicates/chainedHashOpt/chainedHash.h deterministicHash.h;
make clean; make;
export CILK_NWORKERS=1
./suffixTree testfiles/etext99 -c
./suffixTree testfiles/rctail96 -c
./suffixTree testfiles/sprot34.dat -c
