#!/bin/bash
for nt in {1..64}
do
    echo "TH: $nt \n"
    make all NUM_THREADS=$nt
    for t in bomp bthreads
    do
        echo "Prog: $t \n"
        for i in {1..5}
        do
            perf stat ./${t} imgs/romania.bmp 20 1024 res/${t}.bmp
        done
    done
    echo "TH: $nt DONE \n"
done
