#!/bin/bash
make all

for t in bseq bthreads bomp
do
    ./${t} imgs/image1.bmp 20 200 res/${t}.bmp
done
