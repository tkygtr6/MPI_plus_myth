CC = mpicc
CFLAGS = 

all:	sample conc_latency conc_allred overlap

mpi.o:	mpi.h mpi.c
	$(CC) -c mpi.c -lmyth

mpi_debug.o:	mpi.h mpi.c
	$(CC) -o mpi_debug.o -c mpi.c -lmyth -D DEBUG

sample:	sample.c mpi_debug.o
	$(CC) -o sample sample.c mpi_debug.o -lmyth

conc_latency:	benchs/conc_latency.c mpi.o
	$(CC) -o benchs/conc_latency benchs/conc_latency.c mpi.o -lmyth

conc_allred:	benchs/conc_allred.c mpi.o
	$(CC) -o benchs/conc_allred benchs/conc_allred.c mpi.o -lmyth

overlap:	benchs/overlap.c mpi.o
	$(CC) -o benchs/overlap benchs/overlap.c mpi.o -lmyth

clean:
	rm -f *.o sample benchs/conc_latency benchs/conc_allred benchs/overlap

test:
	MYTH_NUM_WORKERS=2 mpiexec -n 2 ./sample 8

run: 
	MYTH_NUM_WORKERS=2 mpiexec -n 2 benchs/conc_latency 16

