#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <myth/myth.h>
#include <unistd.h>
#include <time.h>
#include "../mpi.h"

#define MAX_THREAD 10000
#define MAXSIZE 20000000
#define MAT_SIZE 1024*1024*2
#define NTIMES 1
#define MATTIMES 50
#define MESESAGE_SIZE 8388608

char* sendbuf[MAX_THREAD];
char* recvbuf[MAX_THREAD];
int *A[MAX_THREAD];
int *B[MAX_THREAD];
int *C[MAX_THREAD];
int nthreads;

inline int computation(int thread_rank){
    int i, k;

    for (k=0; k<MATTIMES; k++) {
        for (i=0; i<MAT_SIZE; i++) {
            A[thread_rank][i] = B[thread_rank][i] * C[thread_rank][i];
        }
    }

    return 0;
}

void *overwrap(void *args) {
    int rank, src, dest, tag, i;
    int thread_rank = *(int *)args;

    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    /* All even ranks send to (and recv from) rank i+1 many times */
    tag = thread_rank;
    if ((rank % 2) == 0) { /* even */
        dest = rank + 1;
        for (i=0; i<NTIMES; i++) {
//            printf("i: %d\n", i);
            MPI_Send(sendbuf[thread_rank], MESESAGE_SIZE, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
//            printf("comp:%d\n", myth_get_worker_num());
            computation(thread_rank);
            MPI_Recv(recvbuf[thread_rank], MESESAGE_SIZE, MPI_CHAR, dest, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {  /* odd */
        src = rank - 1;
        for (i=0; i<NTIMES; i++) {
            MPI_Recv(recvbuf[thread_rank], MESESAGE_SIZE, MPI_CHAR, src, nthreads - 1 - tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            computation(thread_rank);
            MPI_Send(sendbuf[thread_rank], MESESAGE_SIZE, MPI_CHAR, src, nthreads - 1 - tag, MPI_COMM_WORLD);
        }
    }

    myth_exit(NULL);
    return 0;
}

int main(int argc, char *argv[]){
    int i, rank, size;
    struct timespec ts_start, ts_end;
    int thread_ranks[MAX_THREAD];

    for(i = 0; i < MAX_THREAD; i++){
        sendbuf[i] = (char *) malloc(sizeof(char) * MAXSIZE);
        recvbuf[i] = (char *) malloc(sizeof(char) * MAXSIZE);
        A[i] = (int *) malloc(sizeof(int) * MAT_SIZE);
        B[i] = (int *) malloc(sizeof(int) * MAT_SIZE);
        C[i] = (int *) malloc(sizeof(int) * MAT_SIZE);
    }

    clock_gettime(CLOCK_REALTIME, &ts_start);

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

    for (i = 0; i < nthreads; i++) {
        thread_ranks[i] = i;
        threads[i] = myth_create(overwrap, (void *)&thread_ranks[i]);
    }

    for(i = 0; i < nthreads; i++){
        myth_join(threads[i], NULL);
    }

    MPI_Finalize();

    clock_gettime(CLOCK_REALTIME, &ts_end);

    long long sec_diff = ts_end.tv_sec - ts_start.tv_sec;
    long long nsec_diff = ts_end.tv_nsec - ts_start.tv_nsec;
    printf("%f\n", (long long) sec_diff + (double) nsec_diff / 1000000000);

    return 0;
}
