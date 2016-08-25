#1 threaded time for chainedHash
cd chainedHashSeparator;
make;
export CILK_NWORKERS=1
./Separator ../graphData/data/3Dgrid_E_10000000
./Separator ../graphData/data/randLocalGraph_E_5_10000000
./Separator ../graphData/data/rMatGraph_E_5_10000000

#80 threaded time for chainedHash
cd ../chainedHashSeparator80;
make;
unset CILK_NWORKERS
./Separator ../graphData/data/3Dgrid_E_10000000
./Separator ../graphData/data/randLocalGraph_E_5_10000000
./Separator ../graphData/data/rMatGraph_E_5_10000000


#1 threaded time for chainedHashOpt
cd ../chainedHashSeparatorOpt;
make;
export CILK_NWORKERS=1
./Separator ../graphData/data/3Dgrid_E_10000000
./Separator ../graphData/data/randLocalGraph_E_5_10000000
./Separator ../graphData/data/rMatGraph_E_5_10000000

#80 threaded time for chainedHashOpt
cd ../chainedHashSeparatorOpt80;
make;
unset CILK_NWORKERS
./Separator ../graphData/data/3Dgrid_E_10000000
./Separator ../graphData/data/randLocalGraph_E_5_10000000
./Separator ../graphData/data/rMatGraph_E_5_10000000

cd ..