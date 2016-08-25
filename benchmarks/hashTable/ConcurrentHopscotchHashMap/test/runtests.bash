#!/bin/bash

rm -f results
rm -f tblResults

percents="8"
threads="1 8 16 24 32 48 56 64"
#threads="64"
algorithms="bhop hopd hopnd chainmmtm chainmtm chainpre"
capacities="8000000"
tblpers="60"

count=0
rep="1 2 3 4 5"
rep="1"

for algorithm in $algorithms; do
for tblper in $tblpers; do
for capacity in $capacities; do
for percent in $percents; do
for thread in $threads; do
for rep1 in $rep; do
	count=$(($count + 1))
	line="$algorithm $count $thread $percent $tblper $capacity 4 1"
	line=`echo $line | sed 's/_/ /g'`
	echo "$line" 1>&2
	echo -n "$line " >> results
        ./test $line  >> results
#	echo >> results
done; done; done; done; done; done;

#cat results | grep lwp_exit | awk '{print $5}' > cache_miss.txt
