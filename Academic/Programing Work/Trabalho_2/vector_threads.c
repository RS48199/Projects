
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



// Define o limite superior para a geração de valores aleatórios
#define LOWER_LIMIT       0
#define UPPER_LIMIT     100

// Define gama de valores a separar em subvector
#define LOWER_VALUE_SUBVECTOR 40 
#define UPPER_VALUE_SUBVECTOR 60





typedef struct M_threadarg
{
	int start;
	int end;
	int* arr;
	int* arrReturn;
	int numOfReturn;
}ThreadArg;





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





int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("Utilização incorreta. Forma correta: ./vector_stat_proc_threads <dimensão_vetor> <número_de_threads>");
        return -1;
    }

	long dimensaoVector = atol(argv[1]);					//Dimensão do vector passado por stdin		=> v_sz
    int numeroThreads=atoi(argv[2]);  					//Numero de processos passado por stdin		=> n_processes
    
    
    
    //Criação de vector
    int* vector = malloc(sizeof(int) * dimensaoVector);
    if(vector == NULL)
    {
		fprintf(stderr, "Erro malloc\n");
		return -1;
	}
	
	
	//Criação de subvector
	long int dimensaoSubVector = dimensaoVector;						//Iniciar subvector com dimensão do vector
	
	int* subvector = malloc(sizeof(int) * dimensaoSubVector);			//Posterior realloc do tamanho do subvector
    if(subvector == NULL)
    {
		fprintf(stderr, "Erro malloc\n");
		return -1;
	}
	
	
	
	//Preenchimento aleatório do vector
	vector_init_rand(vector, dimensaoVector, LOWER_LIMIT, UPPER_LIMIT);


	//Inicio de contagem de tempo de execução
	struct timeval t1,t2;
    gettimeofday(&t1, NULL);


    dimensaoSubVector =  vector_get_in_range_with_threads(vector, dimensaoVector, subvector, LOWER_VALUE_SUBVECTOR, UPPER_VALUE_SUBVECTOR, numeroThreads);


	//FIM do tempo de execução do programa
    gettimeofday(&t2, NULL);
    long elapsed = ((long)t2.tv_sec - t1.tv_sec) * 1000000L + (t2.tv_usec - t1.tv_usec);
    long sec = elapsed / (long)1e6;
    long aux = elapsed % (long)1e6;
    long mil = aux / (long)1e3;
    long mic = aux % (long)1e3;
     

	//Apresentacao ao utilizador
	printf("\nO subvector que apresenta valores entre %d e %d do vector principal tem %ld elementos\n", LOWER_VALUE_SUBVECTOR, UPPER_VALUE_SUBVECTOR, dimensaoSubVector);
	printf ("Elapsed time = %lds%ld,%ldms\n", sec, mil, mic);
	
    free(vector);
    free(subvector);
    

    return 0;
}


