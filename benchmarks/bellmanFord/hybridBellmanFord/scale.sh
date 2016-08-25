dir="graphData/data/"
files=("randLocalGraph_WJ_5_10000000"
    "randLocalGraph_WJ_5_20000000"
    "randLocalGraph_WJ_5_30000000"
    "randLocalGraph_WJ_5_40000000"
    "randLocalGraph_WJ_5_50000000"
    "randLocalGraph_WJ_5_60000000"
    "randLocalGraph_WJ_5_70000000"
    "randLocalGraph_WJ_5_80000000"
    "randLocalGraph_WJ_5_90000000"
    "randLocalGraph_WJ_5_100000000"
)

export CILK_NWORKERS=80


make;
for file in ${files[@]};
do echo "hybridBellmanFord $file";
    ./bellmanFord ../$dir$file
done

# echo "./MVGraph ../graphData/data/randLocalGraph_WJ_5_10000000"
# ./MVGraph ../graphData/data/randLocalGraph_WJ_5_10000000

# make randLocalGraph_WJ_5_90000000;make randLocalGraph_WJ_5_80000000;make randLocalGraph_WJ_5_70000000;make randLocalGraph_WJ_5_60000000;make randLocalGraph_WJ_5_50000000;make randLocalGraph_WJ_5_40000000;make randLocalGraph_WJ_5_30000000;make randLocalGraph_WJ_5_20000000
