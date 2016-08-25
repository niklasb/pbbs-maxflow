dir="graphData/data/"
files=("randLocalGraph_J_5_10000000"
    "randLocalGraph_J_5_20000000"
    "randLocalGraph_J_5_30000000"
    "randLocalGraph_J_5_40000000"
    "randLocalGraph_J_5_50000000"
    "randLocalGraph_J_5_60000000"
    "randLocalGraph_J_5_70000000"
    "randLocalGraph_J_5_80000000"
    "randLocalGraph_J_5_90000000"
    "randLocalGraph_J_5_100000000"
)

apps=("hybridBFS"
"betweennessCentrality"
"radii"
"connectedComponents"
"pagerank")

export CILK_NWORKERS=80

for app in ${apps[@]};
do cd $app; make; cd ..;
for file in ${files[@]};
do echo "$app $file";
    ./$app/MVGraph $dir$file
    ./$app/MVGraph $dir$file
    ./$app/MVGraph $dir$file
done
done
# echo "./MVGraph ../graphData/data/randLocalGraph_J_5_10000000"
# ./MVGraph ../graphData/data/randLocalGraph_J_5_10000000

# make randLocalGraph_J_5_90000000;make randLocalGraph_J_5_80000000;make randLocalGraph_J_5_70000000;make randLocalGraph_J_5_60000000;make randLocalGraph_J_5_50000000;make randLocalGraph_J_5_40000000;make randLocalGraph_J_5_30000000;make randLocalGraph_J_5_20000000
