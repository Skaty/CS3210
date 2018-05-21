CC = nvcc
AFLAGS = -arch=sm_32
CFLAGS = -lcuda -lcudart

all: mm-cuda mm-sm mm-banks

mm-sm: mm-sm.cu
	$(CC) $(AFLAGS) $< -o $@ $(CFLAGS)

mm-cuda: mm-cuda.cu
	$(CC) $(AFLAGS) $< -o $@ $(CFLAGS)

mm-banks: mm-banks.cu
	$(CC) $(AFLAGS) $< -o $@ $(CFLAGS)
