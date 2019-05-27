#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <myth/myth.h>
#include <unistd.h>
#include <assert.h>
#include "mpi.h"
#include "mpi_funcs.h"

cmd_node* cmd_queue_head;
cmd_node* cmd_queue_tail;
myth_thread_t ofl_thread;

req_node *req_manage_list_head;
req_node *req_manage_list_tail;

int rank;

static void ofl_init(void){
    cmd_queue_head = (cmd_node *) malloc(sizeof(cmd_node));
    cmd_queue_head->next = NULL;
    cmd_queue_tail = cmd_queue_head;

    req_manage_list_head = (req_node*) malloc(sizeof(req_node));
    req_manage_list_head->pre = NULL;
    req_manage_list_head->next = NULL;
    req_manage_list_tail = req_manage_list_head;
}

static void ofl_enqueue(cmd_node_contents_t *v){
    cmd_node *q = (cmd_node *) malloc(sizeof(cmd_node));
    q->value = v;
    q->next = NULL;

    cmd_node *p;
    volatile int ret;
    while(1){
        p = cmd_queue_tail;
        if(__sync_bool_compare_and_swap(&p->next, NULL, q)){
            break;
        }
        __sync_bool_compare_and_swap(&cmd_queue_tail, p, p->next);
    }
    __sync_bool_compare_and_swap(&cmd_queue_tail, p, q);
}

static cmd_node* ofl_dequeue(void){
    cmd_node *p;
    do{
        p = cmd_queue_head;
        if(!p->next){
            return NULL;
        }
    }while(!__sync_bool_compare_and_swap(&cmd_queue_head, p, p->next));

    cmd_node *q = p->next;
    return p;
}

static void ofl_force_progress(void){
    if(!req_manage_list_head->next){
        return;
    }
    assert(req_manage_list_head != req_manage_list_tail);

    req_node *node_now = req_manage_list_head->next;
    while(node_now){
        int flag = 0;
        MPI_Status err;
        int error = PMPI_Test(&node_now->req, &flag, &err);
        if (error) printf("error test\n");
        if(flag){
            #ifdef USE_YIELD
                while(__sync_bool_compare_and_swap(node_now->done_flag, 0, 1)){
                    break;
                }
            #endif
            req_node *target_node = node_now;
            node_now->pre->next = node_now->next;
            if (!node_now->next){
                req_manage_list_tail = node_now->pre;
            }else{
                node_now->next->pre = node_now->pre;
            }
            #ifndef USE_YIELD
                myth_uncond_signal(target_node->uncond);
            #endif
            free(target_node);
        }
        node_now = node_now->next;
    }
    #ifdef DEBUG
        printf("fin force_progress\n");
    #endif
}

static void ofl_issue_nonblocking_call(cmd_node *p){
    req_node *n = (req_node *)malloc(sizeof(req_node));
    #ifdef USE_YIELD
        n->done_flag = p->value->done_flag;
    #else
        n->uncond = p->value->uncond;
    #endif
    switch (p->value->name){
        case MPI_Send_name:
        {
            MPI_Send_args_t* args = p->value->args->MPI_Send_args;
            int err;
            err = PMPI_Isend(args->buf, args->count, args->datatype, args->dest, args->tag, args->comm, &n->req);
            if(err) printf("error isend\n");
            #ifdef DEBUG
                printf("Isend finished\n");
            #endif
            break;
        }
        case MPI_Recv_name:
        {
            MPI_Recv_args_t* args = p->value->args->MPI_Recv_args;
            int err;
            err = PMPI_Irecv(args->buf, args->count, args->datatype, args->source, args->tag, args->comm, &n->req);
            if(err) printf("error irecv\n");
            #ifdef DEBUG
                printf("Irecv finished\n");
            #endif
            break;
        }
        case MPI_Bcast_name:
        {
            MPI_Bcast_args_t* args = p->value->args->MPI_Bcast_args;
            PMPI_Ibcast(args->buffer, args->count, args->datatype, args->root, args->comm, &n->req);
            #ifdef DEBUG
                printf("Ibcast finished\n");
            #endif
            break;
        }
        case MPI_Barrier_name:
        {
            MPI_Barrier_args_t* args = p->value->args->MPI_Barrier_args;
            PMPI_Ibarrier(args->comm, &n->req);
            #ifdef DEBUG
                printf("Ibarrier finished\n");
            #endif
            break;
        }
        case MPI_Allgather_name:
        {
            MPI_Allgather_args_t* args = p->value->args->MPI_Allgather_args;
            PMPI_Iallgather(args->sendbuf, args->sendcount, args->sendtype, args->recvbuf, args->recvcount, args->recvtype, args->comm, &n->req);
            #ifdef DEBUG
                printf("Iallgather finished\n");
            #endif
            break;
        }
        case MPI_Allreduce_name:
        {
            MPI_Allreduce_args_t* args = p->value->args->MPI_Allreduce_args;
            PMPI_Iallreduce(args->sendbuf, args->recvbuf, args->count, args->datatype, args->op, args->comm, &n->req);
            #ifdef DEBUG
                printf("Iallreduce finished\n");
            #endif
            break;
        }
        default:
        {
            printf("Error: correspond to none of the MPI functions\n");
            free(n);
            return;
        }
    }

    // append to request management list
    n->pre = req_manage_list_tail;
    n->next = NULL;
    req_manage_list_tail->next = n;
    req_manage_list_tail = n;
}

void *ofl_create(void *args){
    #ifdef DEBUG
        printf("offloading thread created\n");
    #endif

    while(1){
        cmd_node *p = ofl_dequeue();
        if(p){
            ofl_issue_nonblocking_call(p->next);
//            free((void *) p);
        }else{
            ofl_force_progress();
        }
    }

    #ifdef DEBUG
        printf("Error: consume_queue finish\n");
    #endif
}

int MPI_Init(int *argc, char ***argv){
    ofl_init();
    ofl_thread = myth_create(ofl_create, NULL);
    PMPI_Init(argc, argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
}

int MPI_Finalize(void){
    myth_cancel(ofl_thread);
//    myth_fini();
    PMPI_Finalize();
}

#ifdef USE_YIELD
#define D(dummy, name_, Name, tr, num, ...) \
    int MPI_##Name(EXPAND_PARAMS_TO_ARGS(num, Name, __VA_ARGS__)){ \
        cmd_node_contents_t *value = (cmd_node_contents_t *) malloc(sizeof(cmd_node_contents_t)); \
        int done_flag = 0; \
        value->name = MPI_##Name##_name; \
        value->args = (MPI_func_args_t *) malloc(sizeof(MPI_func_args_t)); \
        value->args->MPI_##Name##_args = (MPI_##Name##_args_t *) malloc(sizeof(MPI_##Name##_args_t)); \
        EXPAND_PARAMS_TO_CONTENTS_OF_COMMAND_QUEUE(num, Name, __VA_ARGS__) \
        value->done_flag = &done_flag; \
        \
        ofl_enqueue(value); \
        \
        while(!done_flag){ \
            myth_yield_ex(myth_yield_option_local_first); \
        } \
    };
MPI_FUNCS_ALL(D, )
#undef D

#else
#define D(dummy, name_, Name, tr, num, ...) \
    int MPI_##Name(EXPAND_PARAMS_TO_ARGS(num, Name, __VA_ARGS__)){ \
        cmd_node_contents_t *value = (cmd_node_contents_t *) malloc(sizeof(cmd_node_contents_t)); \
        myth_uncond_t uncond; \
        myth_uncond_init(&uncond); \
        value->name = MPI_##Name##_name; \
        value->uncond = &uncond; \
        value->args = (MPI_func_args_t *) malloc(sizeof(MPI_func_args_t)); \
        value->args->MPI_##Name##_args = (MPI_##Name##_args_t *) malloc(sizeof(MPI_##Name##_args_t)); \
        EXPAND_PARAMS_TO_CONTENTS_OF_COMMAND_QUEUE(num, Name, __VA_ARGS__) \
        \
        ofl_enqueue(value); \
        \
        myth_uncond_wait(&uncond); \
        myth_uncond_destroy(&uncond); \
    };
MPI_FUNCS_ALL(D, )
#undef D
#endif
