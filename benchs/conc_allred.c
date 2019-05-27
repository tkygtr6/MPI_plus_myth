/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *   Copyright (C) 2007 University of Chicago
 *   See COPYRIGHT notice in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "../mpi.h"
#include "myth/myth.h"

#define SIZE 96 // 96, 320, 1920
#define NTIMES 10000
#define MAX_THREADS 1500

/* multithreaded version of conc_allred_th.c */
/* Measures the time taken by concurrent calls to MPI_Allreduce 
   by multiple threads on a node. 
 */

typedef struct args_t_{
    int thread_num;
    myth_barrier_t* barrier;
    MPI_Comm *comm;
} args_t;


void *conc_allred(void *args) {
    int rank, i, color;
    double stime, etime;
    int *inbuf, *outbuf;
    int thread_rank = ((args_t *)args)->thread_num;
    MPI_Comm *comm = ((args_t *) args)->comm;
    myth_barrier_t *barrier = ((args_t *) args)->barrier;

    inbuf = (int *) malloc(SIZE*sizeof(int));
    outbuf = (int *) malloc(SIZE*sizeof(int));

    myth_barrier_wait(barrier);

    stime = MPI_Wtime();
    for (i=0; i<NTIMES; i++) {
        MPI_Allreduce(inbuf, outbuf, SIZE, MPI_INT, MPI_MAX, *(MPI_Comm *)comm);
    }
    etime = MPI_Wtime();

    printf("%f\n", ((etime-stime)*1000)/NTIMES);

    free(inbuf);
    free(outbuf);
}

int main(int argc,char *argv[])
{
    int rank, size, i, provided, nthreads, num_xstreams;
    MPI_Comm comm[MAX_THREADS];

    myth_init();
    myth_thread_t threads[MAX_THREADS];

    // MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (!rank) {
        if (argc != 2) {
            printf("Error: a.out nthreads\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
	
        printf("Time (ms)\n");

        nthreads = atoi(argv[1]);
        MPI_Bcast(&nthreads, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }else{
        MPI_Bcast(&nthreads, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    myth_barrier_t *barrier;
    barrier = (myth_barrier_t *) malloc(sizeof(myth_barrier_t));
    myth_barrier_init(barrier, NULL, nthreads);

    for (i = 0; i < nthreads; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &comm[i]);
        
        args_t * args = (args_t *) malloc(sizeof(args));
        args->barrier = barrier;
        args->thread_num = i;
        args->comm = &comm[i];
        threads[i] = myth_create(conc_allred, (void *)args);
    }

    for(i = 0; i < nthreads; i++){
        myth_join(threads[i], NULL);
    }

    MPI_Finalize();

    return 0;
}


