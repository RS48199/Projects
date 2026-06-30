
// Required
#include <stdlib.h>
#include <stdio.h>

// Threads
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

// Minhas funções
#include "vector_stat_seq.h"
#include "vector_stat_proc_threads.h"












// Função executada pela thread
void* subvector_process_thread(void* argThread)
{
	ThreadArg* arguments = (ThreadArg*)argThread;
	
	int value;
	arguments->numOfReturn = 0;
	for(int i = arguments->start; i <= arguments->end; ++i)
	{
		value = arguments->arr[i];
		if(LOWER_VALUE_SUBVECTOR <= value && value <= UPPER_VALUE_SUBVECTOR)
		{
			arguments->arrReturn[arguments->numOfReturn++] = value;
		}
	}
	
	
	return NULL;
}



//Funcao principal de processamento do array
int vector_get_in_range_with_threads(int v[], int v_sz, int sv[], int min, int max, int n_threads)
{
	
	int step = v_sz / n_threads;
	
	pthread_t* ths = malloc(n_threads * sizeof(pthread_t));
	
	ThreadArg* argThread = malloc(n_threads * sizeof(ThreadArg));
	

	
	for(int i = 0; i < n_threads; i++)
	{
		argThread[i].arr = v;
		argThread[i].arrReturn = malloc(step * sizeof(int));
		argThread[i].start = (i * step);
		argThread[i].end = argThread[i].start + step ;
		
		while(argThread[i].end >= v_sz)
			argThread[i].end --;


		int res = pthread_create(&ths[i],NULL,subvector_process_thread,&(argThread[i]));
		if(res < 0)
		{
			printf("ERROR theread nr %d\n",i);
			return -1;
		}
	}

	//Aguardar fechar todos os processos filho
	for(int i = 0; i <n_threads; i++)
	{
		void* ret;
		pthread_join(ths[i], &ret);
		//printf("Thread %ld ended \n", ths[i]);
	}
	
	
	int idx = 0;
	for(int i = 0; i <n_threads; i++)
	{
		for(int j=0; j< argThread[i].numOfReturn; j++)
		{
			sv[idx] = argThread[i].arrReturn[j];
			
			idx ++;
			
			if(j == argThread[i].numOfReturn)
			{
				free(argThread[i].arrReturn);
			}
		}
		
	}
	
	free(argThread);
	free(ths);
		
	
	return idx;
}
