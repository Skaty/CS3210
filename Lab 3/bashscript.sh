#!/bin/bash

for th in 1 2 4 8 16 32
do
  echo "$th th START"
  for sz in 1 256 512 1024 1536 2048
  do
    echo "Sz: $sz START"
    for i in {1..5}
    do
      perf stat -- ./mm1 $sz $th
    done
    echo "Sz: $sz DONE"
  done
  echo "$th th DONE"
done
