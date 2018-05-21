CC=gcc
NUM_THREADS=64
CFLAGS=-O3 -lm -mavx -D NUM_THREADS=$(NUM_THREADS)

all: seq pthread omp

seq: blur_seq.c
	$(CC) -O3 -o bseq blur_seq.c -lm

pthread: blur_threads.c
	$(CC) -o bthreads blur_threads.c -lpthread $(CFLAGS)

omp: blur_omp.c
	$(CC) -o bomp blur_omp.c -fopenmp $(CFLAGS)
