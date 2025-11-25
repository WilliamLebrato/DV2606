CC = gcc
CFLAGS = -O2 -lpthread

all: gaussianpar gaussianseq qsort qsortseq

gaussianpar: gaussianpar.c
	$(CC) $(CFLAGS) -o $@ $<

gaussianseq: gaussianseq.c
	$(CC) $(CFLAGS) -o $@ $<

qsort: qsort.c
	$(CC) $(CFLAGS) -o $@ $<

qsortseq: qsortseq.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f gaussianpar gaussianseq qsort qsortseq