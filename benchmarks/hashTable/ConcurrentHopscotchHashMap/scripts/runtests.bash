#!/bin/bash

rm -f results
rm -f tblResults

thrd_factor_ary="1"
working_thrd_ary="2"

percents="50_50"
threads="1 2 4 8 12 16 24 32 40 48 56 64"
threads="1 2 4 8 12 16 32 48 56 64 72 80"
#threads="1 2 4 8 12 16 24 32"
algorithms="fcstack lfstack elstack"
algorithms="fclist fclistc fcpq lazylist"
algorithms="fcpqc fcpq"
capacities="30"
#tests="TIME Idle_strands Instr_cnt DC_miss L2_dmiss_ld DTLB_miss CPU_st_to_PCX MMU_ld_to_PCX"
tests="TIME"

rep="1 2"
count=0

for test in $tests; do

rm -f $test

for working_thrd in $working_thrd_ary; do
for thrd_factor in $thrd_factor_ary; do
for algorithm in $algorithms; do
for percent in $percents; do
for capacity in $capacities; do
for thread in $threads; do

        count=$(($count + 1))

        line="$algorithm $count $thread $percent 0.5 20000 10 0 $thrd_factor $working_thrd"
        line=`echo $line | sed 's/_/ /g'`
        echo "$line" 1>&2
        echo -n "$line" >> $test

        for rep1 in $rep; do

           if [ $test == "TIME" ]
             then
              ./test_intel64 $line  >> $test
           fi

           if [ $test != "TIME" ]
             then
              cputrack -T 9999 -e -f -c $test java -Xms768m -Xmx768m -Xss128k -XX:ParallelGCThreads=8 -XX:+UseParNewGC -jar HashBenchmark.jar "$line" | awk '($4 == "exit") {printf " %d",$5}' >> $test
           fi

           cp $test results
           echo -n " " >> results
           echo $test >> results
        done;
        echo >> $test

done; done; done; done; done; done; done;

cat results | grep lwp_exit | awk '($3 > 18) && ($5 > 0) {print $3, $5, $6}' > cache_miss.txt
