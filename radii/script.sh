#maxprocs=80;
procs=(1 2 4 8 16 24 32 40 48);

#procs=(${procs[@]} $(seq 4 4 $maxprocs));

K=(1 2 4 8 16 32 64);

exe="./Diameter ";
yahoo="-2file /home/jshun/data/yahoograph/yahoows.adj /home/jshun/data/yahoograph/yahoows.idx";
rMat100M="-binary -remDups rMatGraph_J_5_100M.bin"
rMat500M="-binary -remDups rMatGraph_J_5_500M.bin"
randLocal100M="-binary -remDups randLocalGraph_J_5_100M.bin"
randLocal500M="-binary -remDups randLocalGraph_J_5_500M.bin"
rMat10M="-binary -remDups rMatGraph_J_5_10M.bin"
randLocal10M="-binary -remDups randLocalGraph_J_5_10M.bin"
randLocal1M="-binary -remDups randLocalGraph_J_5_1M.bin"
rMat1M="-binary -remDups rMatGraph_J_5_1M.bin"


commandsP=(
#    "${yahoo[@]}"
    "${randLocal10M[@]}"
#    "${rMat100M[@]}"
    "${rMat10M[@]}"
    "${rMat1M[@]}"
    "${randLocal1M[@]}"
);

commands=(
 #   "${yahoo[@]}"
    "${randLocal10M[@]}"
#    "${rMat100M[@]}"
    "${rMat10M[@]}"
#    "${rMat500M[@]}"
 #   "${randLocal100M[@]}"
#    "${randLocal500M[@]}"
    "${rMat1M[@]}"
    "${randLocal1M[@]}"
);

fnames=(
 #   "yahoo"
    "randLocal10M"
  #  "rMat100M"
    "rMat10M"
#    "rMat500M"
  #  "randLocal100M"
#    "randLocal500M"
    "rMat1M"
    "randLocal1M"
);

echo ""
echo "runtime vs. processor count for SPMV method"
for c in ${!commandsP[*]}
do
    #echo "${commands[$c]}"
    for p in ${procs[@]}
    do
	
	command="$exe -f 4 ${commandsP[$c]} -cilk_set_worker_count $p"
	echo $command
	eval $command
	#eval $command
	#eval $command
    done
done

echo ""
echo "runtime vs K for SPMV method"
for c in ${!commandsP[*]}
do
    #echo "${commands[$c]}"
    for k in ${K[@]}
    do
	
	command="$exe -f 4 -k $k $out ${commandsP[$c]}"
	echo $command
	eval $command
	#eval $command
	#eval $command
    done
done


echo ""
echo "runtime vs. processor count"
for c in ${!commandsP[*]}
do
    #echo "${commands[$c]}"
    for p in ${procs[@]}
    do
	
	command="$exe ${commandsP[$c]} -cilk_set_worker_count $p"
	echo $command
	eval $command
	#eval $command
	#eval $command
    done
done




echo ""
echo "runtime and accuracy vs K"
for c in ${!commandsP[*]}
do
    #echo "${commands[$c]}"
    for k in ${K[@]}
    do
	
	out="-o timings/${fnames[$c]}_k$k"
	command="$exe -k $k $out ${commandsP[$c]}"
	echo $command
	eval $command
	#eval $command
	#eval $command
    done
done

# echo ""
# echo "run once for all the inputs"
# for c in ${!commands[*]}
# do	
#     out="-o timings/${fnames[$c]}_64"
#     command="$exe $out ${commands[$c]}"
#     echo $command
#     eval $command
#     #eval $command
#     #eval $command
# done

echo ""
echo "runtime and accuracy of bitString vs. K"
for c in ${!commandsP[*]}
do

    for k in ${K[@]}
    do
	out="-o timings/${fnames[$c]}_bitString_k$k"
	command="$exe -k $k $out -f 2 ${commandsP[$c]}"
	echo $command
	eval $command
    done
done

echo ""
echo "runtime vs. proc count for bitString"
for c in ${!commandsP[*]}
do
    for p in ${procs[@]}
    do
	command="$exe -k 64 -f 2 ${commandsP[$c]} -cilk_set_worker_count $p"
	echo $command
	eval $command
    done
done

echo ""
echo "timing regular BFS -- time vs proc count for k=1,64"
for c in ${!commands[*]}
do
    for p in ${procs[@]}
    do	
	command="$exe -k 64 -f 3 ${commands[$c]} -cilk_set_worker_count $p"
	echo $command
	eval $command
	command="$exe -k 1 -f 3 ${commands[$c]} -cilk_set_worker_count $p"
	echo $command
	eval $command
    done
done

# echo "$exe -k 64 -f 3 $randLocal10M"
# eval "$exe -k 64 -f 3 $randLocal10M"

# echo "$exe -k 1 -f 3 $randLocal10M"
# eval "$exe -k 1 -f 3 $randLocal10M"

# echo "$exe -k 64 -f 3 $yahoo"
# eval "$exe -k 64 -f 3 $yahoo"
