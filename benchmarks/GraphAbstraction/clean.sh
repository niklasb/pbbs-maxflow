#!/bin/sh

for dir in *
do
    if [ "$dir" != "graphData" ]; then
	(cd $dir && make cleansrc)
    fi
done
