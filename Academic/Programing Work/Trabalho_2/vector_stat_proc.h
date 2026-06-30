
#ifndef __VECTOR_STAT_PROC_H__
#define __VECTOR_STAT_PROC_H__

	// Define o limite superior para a geração de valores aleatórios
	#define LOWER_LIMIT       0
	#define UPPER_LIMIT     100

	// Define gama de valores a separar em subvector
	#define LOWER_VALUE_SUBVECTOR 40 
	#define UPPER_VALUE_SUBVECTOR 60

	// Função com processamento do Child
	void subvector_process(int start, int stop, int v[], int pipeChildParent[]);
	//Funcao principal de processamento do array
	int vector_get_in_range(int v[], int v_sz, int sv[], int min, int max, int n_processes);


#endif

