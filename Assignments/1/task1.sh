#!/bin/bash

make all

for t in bseq bthreads bomp
do
    echo "Program: $t \n"
    for ksize in 300 250 200 150 100 50
    do
        echo "ksize: $ksize \n"
        for i in {1..5}
        do
            perf stat ./${t} imgs/romania.bmp 20 $ksize res/${t}.bmp
        done
        echo "[DONE] ksize: $ksize \n"
    done
    echo "[DONE] Program: $t \n"
done
