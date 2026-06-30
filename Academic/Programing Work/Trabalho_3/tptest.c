#include "thp.h"
#include <stdio.h>
#include <unistd.h>

#define NR_OF_THREADS (3)
#define MAX_ARGS (10)


void *ft(void * arg)
{
  int id = *(int*) arg;
  printf("%d enter on thread %ld\n",id, pthread_self() );
  sleep(2);
  printf("%d exit on thread %ld\n",id, pthread_self() );
  
  return NULL;
}

int main()
{
  threadpool_t tp;
  threadpool_init(&tp, MAX_ARGS, NR_OF_THREADS);
  
  for(int i=0; i<MAX_ARGS; i++)
  {
		if(i==5){threadpool_destroy(&tp);
	}
  threadpool_submit(&tp, ft,&i);
  sleep(1);
  }

}
