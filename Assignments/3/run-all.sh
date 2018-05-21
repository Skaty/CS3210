#!/bin/bash
for sz in 32 64 128 256 512 1024 2048
do
  echo "$sz START"
  for prog in mm-seq mm-cuda mm-sm mm-banks
  do
    echo "$prog START"
    ./$prog $sz
    echo "$prog DONE"
  done
  echo "$sz DONE"
done