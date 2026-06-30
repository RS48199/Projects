#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "count_down.h"
#define MAX_THREADS 8
#define COUNTDOWN 4


int countdown_init (countdown_t *cd, int initialValue){
  cd->value = initialValue;
  pthread_mutex_init(&(cd->lock), NULL);
  pthread_cond_init(&(cd->wait), NULL);
  return 1;
}
int countdown_destroy (countdown_t *cd){
    pthread_mutex_destroy(&(cd->lock));
    pthread_cond_destroy(&(cd->wait));
   return 1;
}
int countdown_wait (countdown_t *cd){
  pthread_mutex_lock(&(cd->lock));
  while(cd->value > 0){
    pthread_cond_wait(&(cd->wait), &(cd->lock));
  }
  pthread_mutex_unlock(&(cd->lock));
  return 1;
  
}
int countdown_down (countdown_t *cd){
  pthread_mutex_lock(&(cd->lock));
  cd->value--;
  pthread_cond_broadcast(&(cd->wait));
  pthread_mutex_unlock(&(cd->lock));
  return cd->value;
}

void* threadWork(void *arg){
  countdown_t * cd = (countdown_t *)arg;
  pthread_t id =  pthread_self();
  int sec=8;
  printf("Thread %d is working for %d seconds\n", (int)id, sec );
  sleep(sec);
  if(countdown_down(cd)!=cd->value){
    printf("Error on countdown_t\n");
  }
  if(cd->value>=0){
    printf("Thread %d: Waiting all threads to finish\n", (int)id);
    countdown_wait(cd);
  }else{
	   printf("Thread %d: ERROR: Out of scope of countdown_t, ending following threads\n", (int)id);
	   pthread_exit(NULL);
  }
  printf("Thread %d: Finnished \n", (int)id);
  return NULL;
}

int test_main(){
  pthread_t ths[MAX_THREADS];
  countdown_t cd;
  if(countdown_init(&cd, COUNTDOWN)!=1){
    printf("Error initializing countdown_t\n");
    return -1;
  };
  for(int i=0; i<MAX_THREADS; i++){
    pthread_create(ths+i, NULL, threadWork, &cd);
    sleep(1);
  }
  for(int i=0; i<MAX_THREADS; i++){
    pthread_join(ths[i], NULL);
  }
  printf("---------------------\nAll Threads Finished\n---------------------\n");
  if(countdown_destroy(&cd)==0){
    return 0;
  }
  return -1;
}
