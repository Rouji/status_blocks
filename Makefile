CC=gcc
CFLAGS=-O3

all: cpubars bandwidth

cpubars: cpubars.c
	$(CC) -o cpubars cpubars.c

bandwidth: bandwidth.c
	$(CC) -o bandwidth bandwidth.c

clean:
	rm cpubars bandwidth
