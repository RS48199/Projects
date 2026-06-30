#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include "thp.h"



// Função que será executada por cada thread do pool
void *worker_thread(void *arg) {
    threadpool_t *tp = (threadpool_t *)arg;
    while (true) {
        pthread_mutex_lock(&tp->lock);

        while (tp->count == 0 && !tp->stop) {
            pthread_cond_wait(&tp->cond_var, &tp->lock);
        }

        if (tp->stop && tp->count == 0) {
            pthread_mutex_unlock(&tp->lock);
            break;
        }

        work_item_t work = tp->queue[tp->head];
        tp->head = (tp->head + 1) % tp->size;
        tp->count--;

        tp->active_ths++;

        pthread_mutex_unlock(&tp->lock);

        work.func(work.arg);

        pthread_mutex_lock(&tp->lock);
        tp->active_ths--;
        if (tp->stop && tp->count == 0 && tp->active_ths == 0) {
            pthread_cond_signal(&tp->cond_complete);
        }
        pthread_mutex_unlock(&tp->lock);
    }
    return NULL;
}

// Inicia o thread pool
int threadpool_init(threadpool_t *tp, int queueDim, int nthreads) {
    tp->size = queueDim;
    tp->count = 0;
    tp->head = 0;
    tp->tail = 0;
    tp->nthreads = nthreads;
    tp->stop = false;
    tp->active_ths = 0;
    tp->ths = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
    tp->queue = (work_item_t *)malloc(queueDim * sizeof(work_item_t));
    pthread_mutex_init(&tp->lock, NULL);
    pthread_cond_init(&tp->cond_var, NULL);
    pthread_cond_init(&tp->cond_complete, NULL);

    for (int i = 0; i < nthreads; i++) {
        pthread_create(&tp->ths[i], NULL, worker_thread, tp);
    }

    return 0;
}

// Submete um trabalho ao thread pool
int threadpool_submit(threadpool_t *tp, wi_function_t func, void *args) {
    pthread_mutex_lock(&tp->lock);

    // Verifica se a fila está cheia
    if (tp->count == tp->size) {
        pthread_mutex_unlock(&tp->lock);
        return -1;  // Fila cheia
    }

    // Adiciona o trabalho à fila
    tp->queue[tp->tail].func = func;
    tp->queue[tp->tail].arg = args;
    tp->tail = (tp->tail + 1) % tp->size;
    tp->count++;

    pthread_cond_signal(&tp->cond_var);
    pthread_mutex_unlock(&tp->lock);

    return 0;
}

// Destroi o thread pool
int threadpool_destroy(threadpool_t *tp) {
    pthread_mutex_lock(&tp->lock);
    tp->stop = true;
    pthread_cond_broadcast(&tp->cond_var);

    while (tp->count > 0 || tp->active_ths > 0) {
        pthread_cond_wait(&tp->cond_complete, &tp->lock);
    }
    pthread_mutex_unlock(&tp->lock);

    for (int i = 0; i < tp->nthreads; i++) {
        printf("%ld exitting\n", pthread_self());
        pthread_join(tp->ths[i], NULL);
    }

    free(tp->ths);
    free(tp->queue);

    pthread_mutex_destroy(&tp->lock);
    pthread_cond_destroy(&tp->cond_var);
    pthread_cond_destroy(&tp->cond_complete);

    return 0;
}
 
