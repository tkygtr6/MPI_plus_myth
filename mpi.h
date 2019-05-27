#pragma once

#include <mpi.h>
#include <myth/myth.h>
#include "mpi_funcs.h"

//#define USE_YIELD

typedef struct cmd_node_contents{
    MPI_func_name_t name;
    MPI_func_args_t *args;
    #ifdef USE_YIELD
       int *done_flag;
    #else
        myth_uncond_t *uncond;
    #endif
} cmd_node_contents_t;

typedef volatile struct cmd_node_{
    cmd_node_contents_t *value;
    volatile struct cmd_node_ *next;
} cmd_node;

typedef struct req_node_{
    MPI_Request req;
    #ifdef USE_YIELD
        int *done_flag;
    #else
        myth_uncond_t *uncond;
    #endif
    struct req_node_ *pre;
    struct req_node_ *next;
} req_node;

static void ofl_init(void);
static void ofl_enqueue(cmd_node_contents_t *v);
static cmd_node* ofl_dequeue(void);
static void ofl_force_progress(void);
static void ofl_issue_nonblocking_call(cmd_node *p);
void *ofl_create(void *args);

int MPI_Init(int *argc, char ***argv);
int MPI_Finalize(void);

/*
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status* status);
*/
#define D(dummy, name, Name, tr, num, ...) \
    int MPI_##Name(EXPAND_PARAMS_TO_ARGS(num, Name, __VA_ARGS__));
MPI_FUNCS_ALL(D, )
#undef D
