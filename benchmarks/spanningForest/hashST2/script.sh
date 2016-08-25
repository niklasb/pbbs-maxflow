# make cleansrc; make

# unset CILK_NWORKERS
# ./ST ../graphData/data/3Dgrid_E_10000000
# ./ST ../graphData/data/randLocalGraph_E_5_10000000
# ./ST ../graphData/data/rMatGraph_E_5_10000000

# export CILK_NWORKERS=1
# ./ST ../graphData/data/3Dgrid_E_10000000
# ./ST ../graphData/data/randLocalGraph_E_5_10000000
# ./ST ../graphData/data/rMatGraph_E_5_10000000

make cleansrc; 
ln -s ../../removeDuplicates/ndHash/ndHash.h deterministicHash.h
make

unset CILK_NWORKERS
./ST ../graphData/data/3Dgrid_E_10000000
./ST ../graphData/data/randLocalGraph_E_5_10000000
./ST ../graphData/data/rMatGraph_E_5_10000000

export CILK_NWORKERS=1
./ST ../graphData/data/3Dgrid_E_10000000
./ST ../graphData/data/randLocalGraph_E_5_10000000
./ST ../graphData/data/rMatGraph_E_5_10000000

# make cleansrc; 
# ln -s ../../removeDuplicates/cuckooHash/cuckooHash.h deterministicHash.h
# make

# unset CILK_NWORKERS
# ./ST ../graphData/data/3Dgrid_E_10000000
# ./ST ../graphData/data/randLocalGraph_E_5_10000000
# ./ST ../graphData/data/rMatGraph_E_5_10000000

# export CILK_NWORKERS=1
# ./ST ../graphData/data/3Dgrid_E_10000000
# ./ST ../graphData/data/randLocalGraph_E_5_10000000
# ./ST ../graphData/data/rMatGraph_E_5_10000000
