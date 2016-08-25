unset CILK_NWORKERS
./Separator ../graphData/data/3Dgrid_E_10000000
./Separator ../graphData/data/randLocalGraph_E_5_10000000
./Separator ../graphData/data/rMatGraph_E_5_10000000

export CILK_NWORKERS=1
./Separator ../graphData/data/3Dgrid_E_10000000
./Separator ../graphData/data/randLocalGraph_E_5_10000000
./Separator ../graphData/data/rMatGraph_E_5_10000000
