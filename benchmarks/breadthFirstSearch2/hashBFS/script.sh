unset CILK_NWORKERS
./BFS ../graphData/data/3Dgrid_J_10000000
./BFS ../graphData/data/randLocalGraph_J_5_10000000
./BFS ../graphData/data/rMatGraph_J_5_10000000

export CILK_NWORKERS=1
./BFS ../graphData/data/3Dgrid_J_10000000
./BFS ../graphData/data/randLocalGraph_J_5_10000000
./BFS ../graphData/data/rMatGraph_J_5_10000000
