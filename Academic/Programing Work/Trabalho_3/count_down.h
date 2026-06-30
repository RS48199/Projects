#ifndef _CDOWN_T_
#define _CDOWN_T_
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
 int value;
 pthread_mutex_t lock;
 pthread_cond_t wait;
} countdown_t;

int countdown_init (countdown_t *cd, int initialValue);
int countdown_destroy (countdown_t *cd);
int countdown_wait (countdown_t *cd);
int countdown_down (countdown_t *cd);

#endif
