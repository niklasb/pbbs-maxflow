dir="data/"
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

wfiles=("randLocalGraph_WJ_5_10000000"
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

apps=("BFS"
"BC"
"Radii"
"Components"
"PageRank")

wapps=("BellmanFord")

export CILK_NWORKERS=80

for app in ${apps[@]};
do 
for file in ${files[@]};
do echo "$app $file";
    ./$app $dir$file -s
    echo " "
    ./$app $dir$file -s
    echo " "
    ./$app $dir$file -s
    echo " "
done
done


# for app in ${wapps[@]};
# do 
# for file in ${wfiles[@]};
# do echo "$app $file";
#     ./$app $dir$file -s
#     echo " "
#     ./$app $dir$file -s
#     echo " "
#     ./$app $dir$file -s
#     echo " "
# done
# done


# make randLocalGraph_J_5_100000000; make randLocalGraph_J_5_90000000;make randLocalGraph_J_5_80000000;make randLocalGraph_J_5_70000000;make randLocalGraph_J_5_60000000;make randLocalGraph_J_5_50000000;make randLocalGraph_J_5_40000000;make randLocalGraph_J_5_30000000;make randLocalGraph_J_5_20000000; make randLocalGraph_J_5_10000000;

# make randLocalGraph_WJ_5_100000000; make randLocalGraph_WJ_5_90000000;make randLocalGraph_WJ_5_80000000;make randLocalGraph_WJ_5_70000000;make randLocalGraph_WJ_5_60000000;make randLocalGraph_WJ_5_50000000;make randLocalGraph_WJ_5_40000000;make randLocalGraph_WJ_5_30000000;make randLocalGraph_WJ_5_20000000; make randLocalGraph_WJ_5_10000000;
