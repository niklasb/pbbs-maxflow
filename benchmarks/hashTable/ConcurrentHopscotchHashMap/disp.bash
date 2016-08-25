#!/bin/bash

results="$1results"

COUNTER=0
while [  $COUNTER -lt 10 ]; do
   clear
   cat $results
   sleep 6
done;
