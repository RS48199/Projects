#ifndef _THP_
#define _THP_
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

// Definição da função de trabalho
typedef void *(*wi_function_t)(void *);

// Estrutura para representar um trabalho
typedef struct {
    wi_function_t func;
    void *arg;
} work_item_t;

//Thread_pool
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond_var;
    pthread_cond_t cond_complete;
    pthread_t *ths;
    work_item_t *queue;
    int size;
    int count;
    int head;
    int tail;
    int nthreads;
    bool stop;
    int active_ths;
} threadpool_t;

int threadpool_init(threadpool_t *tp, int queueDim, int nthreads);
int threadpool_submit(threadpool_t *tp, wi_function_t func, void *args);
int threadpool_destroy(threadpool_t *tp);
 
#endif
