
#ifndef __PROC_CONNECTION_H__
#define __PROC_CONNECTION_H__

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
	
	
	void* subvector_process_thread(void* argThread);
	int vector_get_in_range_with_threads(int v[], int v_sz, int sv[], int min, int max, int n_threads);
	//int vector_proc_threads(int sockfd, long dimensaoVector, int numeroThreads);
	

#endif 

