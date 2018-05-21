#!/bin/bash

make all

for t in bseq
do
    echo "Prog: $t \n"
    for ksize in 300 250 200 150 100 50
    do
        echo "Sz: $ksize \n"
        for i in {1..5}
        do
            perf stat ./${t} imgs/romania.bmp 20 $ksize res/${t}.bmp
        done
        echo "Sz: $ksize done \n"
    done

    echo "Task 2: 64 \n"
    echo "Proc sz: 0 \n"
    for i in {1..5}
    do
        perf stat numactl --physcpubind=0 ./${t} imgs/romania.bmp 20 1024 res/${t}.bmp
    done
    echo "Task 2: 64 done \n"
done

for t in bthreads bomp
do
    echo "Prog: $t \n"
    for ksize in 300 250 200 150 100 50
    do
        echo "Sz: $ksize \n"
        for i in {1..5}
        do
            perf stat ./${t} imgs/romania.bmp 20 $ksize res/${t}.bmp
        done
        echo "Sz: $ksize done \n"
    done

    echo "Task 2: 1024 \n"
    for sz in 0 1 3 5 7
    do
        echo "Proc sz: $sz \n"
        for i in {1..5}
        do
            perf stat numactl --physcpubind=0-${sz} ./${t} imgs/romania.bmp 20 1024 res/${t}.bmp
        done
    done
    echo "Task 2: 1024 done \n"
done
