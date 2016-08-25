#!/bin/bash

results="$1results"

rm -f $results
rm -f tblResults

percents="$2"
threads="1 8 16 24 32 48 56 64"
algorithms="bhop hopd hopnd chainmmtm chainmtm chainpre"
capacities="16000000"
tblpers="60"

tests="TIME Idle_strands Instr_cnt DC_miss L2_dmiss_ld DTLB_miss CPU_st_to_PCX MMU_ld_to_PCX"
tests="TIME"

count=0
rep="1 2 3 4 5"
rep="1"

for test in $tests; do

rm -f "$1$test"

for algorithm in $algorithms; do
for capacity in $capacities; do

for tblper in $tblpers; do
for percent in $percents; do

for thread in $threads; do

        count=$(($count + 1))

        line="$algorithm $count $thread $percent $tblper $capacity 4 1"
        line=`echo $line | sed 's/_/ /g'`
        echo "$line" 1>&2
        echo -n "$line" >> "$1$test"

        for rep1 in $rep; do

           if [ $test == "TIME" ]
             then
               $1 $line  >> "$1$test"
           fi

           if [ $test != "TIME" ]
             then
              cputrack -T 9999 -e -f -c $test $1" $line" | awk '($4 == "exit") {printf " %d",$5}' >> "$1$test"
           fi

           cp "$1$test" $results
           echo -n " " >> $results
           echo "$1$test" >> $results
        done;
        echo >> "$1$test"

done; done; done; done; done; done;


cat $results | grep lwp_exit | awk '($3 > 18) && ($5 > 0) {print $3, $5, $6}' > cache_miss.txt

