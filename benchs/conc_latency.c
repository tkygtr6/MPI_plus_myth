#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <myth/myth.h>
#include <unistd.h>
#include "../mpi.h"

#define MAX_THREAD 1500
#define MAXSIZE 2048
#define NTIMES 1000

typedef struct args_t_{
    int thread_num;
    myth_barrier_t* barrier;
} args_t;

void *conc_latency(void *args) {
    int rank, src, dest, tag, i, size, incr;
    double stime, etime, ttime;
    char *sendbuf, *recvbuf;
    int thread_rank = ((args_t *)args)->thread_num;
    myth_barrier_t *barrier = ((args_t *) args)->barrier;

    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    sendbuf = (char *) malloc(MAXSIZE);
    recvbuf = (char *) malloc(MAXSIZE);

    myth_barrier_wait(barrier);

    /* All even ranks send to (and recv from) rank i+1 many times */
    incr = 32;
    tag = thread_rank;
    if ((rank % 2) == 0) { /* even */
        dest = rank + 1;

        if (thread_rank == 0){
            printf("Size (bytes) \t Time (us)\n");
        }

        for (size=0; size<=MAXSIZE; size+=incr) {
            myth_barrier_wait(barrier);
            stime = MPI_Wtime();
            for (i=0; i<NTIMES; i++) {
                MPI_Send(sendbuf, size, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
                MPI_Recv(recvbuf, size, MPI_CHAR, dest, tag, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            }
            etime = MPI_Wtime();

            ttime = (etime - stime)/(2*NTIMES);

            if (thread_rank == 0)
                printf("%d \t %f\n", size, ttime*1000000);

            if (size == 512) incr = 128;
        }
    } else {  /* odd */
        src = rank - 1;

        for (size=0; size<=MAXSIZE; size+=incr) {
            myth_barrier_wait(barrier);
            for (i=0; i<NTIMES; i++) {
                MPI_Recv(recvbuf, size, MPI_CHAR, src, tag, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
                MPI_Send(sendbuf, size, MPI_CHAR, src, tag, MPI_COMM_WORLD);
            }
            if (size == 512) incr = 128;
        }
    }

    free(sendbuf);
    free(recvbuf);
    myth_exit(NULL);
    return 0;
}

int main(int argc, char *argv[]){
    int i, rank, size, nthreads;

    myth_init();
    myth_thread_t threads[MAX_THREAD];

//    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        printf("Run with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (!rank) {
        if (argc != 2) {
            printf("Error: a.out nthreads\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        nthreads = atoi(argv[1]);
        MPI_Send(&nthreads, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }else{
        MPI_Recv(&nthreads, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    myth_barrier_t *barrier;
    barrier = (myth_barrier_t *) malloc(sizeof(myth_barrier_t));
    myth_barrier_init(barrier, NULL, nthreads);

    for (i = 0; i < nthreads; i++) {
        args_t * args = (args_t *) malloc(sizeof(args));
        args->barrier = barrier;
        args->thread_num = i;
        threads[i] = myth_create(conc_latency, (void *)args);
    }

    for(i = 0; i < nthreads; i++){
        myth_join(threads[i], NULL);
    }

    MPI_Finalize();

    return 0;
}
