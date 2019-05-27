#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <myth/myth.h>
#include <unistd.h>
#include "mpi.h"

#define MAX_THREAD 100000
#define MAXSIZE 2048
#define NTIMES 1000


void *send_func(void *args){
    char *buf;
    int thread_rank = *(int *)args;
    int BUFSIZE = 100;
    buf = (char *) malloc(sizeof(char) * BUFSIZE);
    sprintf(buf, "Hello World from thread id %d", thread_rank);
    MPI_Send(buf, BUFSIZE, MPI_BYTE, 1, 0, MPI_COMM_WORLD);

    myth_exit(NULL);
}

void *recv_func(void *args){
    char *buf;
    int thread_rank = *(int *)args;
    int BUFSIZE = 100;
    buf = (char *) malloc(sizeof(char) * BUFSIZE);
    MPI_Recv(buf, BUFSIZE, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    printf("%d: buf: %s\n", thread_rank, buf);

    myth_exit(NULL);
}

int main(int argc, char *argv[]){
    int i, rank, size, nthreads;
    int thread_num[MAX_THREAD];

    myth_init();
    myth_thread_t threads[MAX_THREAD];

    //ret = MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
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

    for (i = 0; i < nthreads; i++) {
        thread_num[i] = i;
        if(rank) {
            threads[i] = myth_create(recv_func, (void *)&thread_num[i]);
        }else{
            threads[i] = myth_create(send_func, (void *)&thread_num[i]);
        }
    }

    for(i = 0; i < nthreads; i++){
        myth_join(threads[i], NULL);
    }

    MPI_Finalize();
}
