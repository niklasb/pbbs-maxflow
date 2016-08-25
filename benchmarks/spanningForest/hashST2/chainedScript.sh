#1 threaded time for chainedHash
make cleansrc;
ln -s ../../removeDuplicates/chainedHash/chainedHash.h deterministicHash.h
make;

export CILK_NWORKERS=1
./ST ../graphData/data/3Dgrid_E_10000000
./ST ../graphData/data/randLocalGraph_E_5_10000000
./ST ../graphData/data/rMatGraph_E_5_10000000

#80 threaded time for chainedHash
make cleansrc;
ln -s ../../removeDuplicates/chainedHash80/chainedHash.h deterministicHash.h
make;
unset CILK_NWORKERS
./ST ../graphData/data/3Dgrid_E_10000000
./ST ../graphData/data/randLocalGraph_E_5_10000000
./ST ../graphData/data/rMatGraph_E_5_10000000


#1 threaded time for chainedHashOpt
make cleansrc;
ln -s ../../removeDuplicates/chainedHashOpt/chainedHash.h deterministicHash.h
make;

export CILK_NWORKERS=1
./ST ../graphData/data/3Dgrid_E_10000000
./ST ../graphData/data/randLocalGraph_E_5_10000000
./ST ../graphData/data/rMatGraph_E_5_10000000

#80 threaded time for chainedHashOpt
make cleansrc;
ln -s ../../removeDuplicates/chainedHashOpt80/chainedHash.h deterministicHash.h
make;
unset CILK_NWORKERS
./ST ../graphData/data/3Dgrid_E_10000000
./ST ../graphData/data/randLocalGraph_E_5_10000000
./ST ../graphData/data/rMatGraph_E_5_10000000
