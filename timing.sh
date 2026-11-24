#!/bin/bash
# Name: Konrad Swierczek
# Assignment: CPE-2600 Lab 11 - Multiprocessing
# Section: 111
# Description: Times mandelmovie for several process counts and writes
#              results to timing.csv (processes,seconds).
#              Run from the repo directory after 'make'.

echo "processes,seconds" > timing.csv
for p in 1 2 5 10 20; do
    rm -f mandel*.jpg
    start=$(date +%s.%N)
    ./mandelmovie -p "$p" > /dev/null
    end=$(date +%s.%N)
    secs=$(echo "$end - $start" | bc)
    echo "$p,$secs" | tee -a timing.csv
done
