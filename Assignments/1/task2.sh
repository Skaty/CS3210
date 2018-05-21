#!/bin/bash

make all

for t in bseq
do
    echo "Program: bseq"
    echo "Proc Sz: 0"
    for i in {1..5}
    do
        perf stat numactl --physcpubind=0 ./${t} imgs/romania.bmp 20 1024 res/${t}.bmp
    done
    echo "Program: bseq"
done

for t in bthreads bomp
do
    echo "Program: $t"
    for sz in 0 1 3 5 7
    do
        echo "Proc Sz: $sz"
        for i in {1..5}
        do
            perf stat numactl --physcpubind=0-${sz} ./${t} imgs/romania.bmp 20 1024 res/${t}.bmp
        done
        echo "[DONE] Proc Sz: $sz"
    done
done
