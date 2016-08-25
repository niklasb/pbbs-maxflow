#1 threaded time for chainedHash
make cleansrc;
ln -s ../../removeDuplicates/chainedHash/chainedHash.h deterministicHash.h
make;

export CILK_NWORKERS=1
./BFS ../graphData/data/3Dgrid_J_10000000
./BFS ../graphData/data/randLocalGraph_J_5_10000000
./BFS ../graphData/data/rMatGraph_J_5_10000000

#80 threaded time for chainedHash
make cleansrc;
ln -s ../../removeDuplicates/chainedHash80/chainedHash.h deterministicHash.h
make;
unset CILK_NWORKERS
./BFS ../graphData/data/3Dgrid_J_10000000
./BFS ../graphData/data/randLocalGraph_J_5_10000000
./BFS ../graphData/data/rMatGraph_J_5_10000000


#1 threaded time for chainedHashOpt
make cleansrc;
ln -s ../../removeDuplicates/chainedHashOpt/chainedHash.h deterministicHash.h
make;

export CILK_NWORKERS=1
./BFS ../graphData/data/3Dgrid_J_10000000
./BFS ../graphData/data/randLocalGraph_J_5_10000000
./BFS ../graphData/data/rMatGraph_J_5_10000000

#80 threaded time for chainedHashOpt
make cleansrc;
ln -s ../../removeDuplicates/chainedHashOpt80/chainedHash.h deterministicHash.h
make;
unset CILK_NWORKERS
./BFS ../graphData/data/3Dgrid_J_10000000
./BFS ../graphData/data/randLocalGraph_J_5_10000000
./BFS ../graphData/data/rMatGraph_J_5_10000000
