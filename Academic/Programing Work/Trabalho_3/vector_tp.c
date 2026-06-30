#include "vector_tp.h"
#include "thp.h"
#include "count_down.h"
#include <stdio.h>
#include <stdlib.h>

#define N_THS 5
#define MIN_VAL 5
#define MAX_VAL 50
#define V_SZ 500
#define BUFF 100

typedef struct {
    int *v;
    int start;
    int end;
    int min;
    int max;
    int *sv;
    int *sv_sz;
    countdown_t *cd;
    pthread_mutex_t *sv_lock;
} work_item_args_t;

void* work_function(void *arg) {
    work_item_args_t *args = (work_item_args_t *)arg;
    int *v = args->v;
    int start = args->start;
    int end = args->end;
    int min = args->min;
    int max = args->max;
    int *sv = args->sv;
    int *sv_sz = args->sv_sz;
    pthread_mutex_t *sv_lock = args->sv_lock;

    for (int i = start; i < end; i++) {
        if (v[i] >= min && v[i] <= max) {
            pthread_mutex_lock(sv_lock);
            sv[*sv_sz] = v[i];
            (*sv_sz)++;
            pthread_mutex_unlock(sv_lock);
        }
    }

    countdown_down(args->cd);
    free(arg);
    return NULL;
}

int vector_get_in_range_with_thread_pool(int v[], int v_sz, int sv[], int min, int max, threadpool_t *tp) {
    int nthreads = tp->nthreads;
    int chunk_size = (v_sz + nthreads - 1) / nthreads;
    int sv_sz = 0;
    pthread_mutex_t sv_lock;
    pthread_mutex_init(&sv_lock, NULL);

    countdown_t cd;
    countdown_init(&cd, nthreads);

    for (int i = 0; i < nthreads; i++) {
        int start = i * chunk_size;
        int end = (start + chunk_size > v_sz) ? v_sz : start + chunk_size;

        if (start >= v_sz) break;

        work_item_args_t *args = (work_item_args_t *)malloc(sizeof(work_item_args_t));
        args->v = v;
        args->start = start;
        args->end = end;
        args->min = min;
        args->max = max;
        args->sv = sv;
        args->sv_sz = &sv_sz;
        args->cd = &cd;
        args->sv_lock = &sv_lock;

        threadpool_submit(tp, work_function, (void *)args);
    }

    countdown_wait(&cd);

    countdown_destroy(&cd);
    pthread_mutex_destroy(&sv_lock);

    return sv_sz;
}

int get_random (int min, int max) 
{
    return rand() % (max - min + 1) + min;
}

void vector_init_rand (int v[], long dim, int min, int max)
{
    for (long i = 0; i < dim; i++) {
        v[i] = get_random(min, max);
    }
}

void vector_get_incremental(int v[], long dim){
    for (long i = 0; i < dim; i++) {
        v[i] = i / 10;
    }
}

int main() {
    int v[V_SZ];
    int v_sz = V_SZ;
    int min = MIN_VAL / 2;
    int max = MAX_VAL / 2;
    int sv[V_SZ]; 
    int sv_sz;
    vector_get_incremental(v, v_sz);

    threadpool_t tp;
    int nthreads = N_THS;
    int queue_size = BUFF;

    threadpool_init(&tp, queue_size, nthreads);

    sv_sz = vector_get_in_range_with_thread_pool(v, v_sz, sv, min, max, &tp);

    threadpool_destroy(&tp);

    printf("Subvector with %d elements in range [%d, %d]:\n", sv_sz, min, max);
    for (int i = 0; i < sv_sz; i++) {
        printf("%d ", sv[i]);
    }
    printf("\n");

    return 0;
}
