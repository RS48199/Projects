// Required
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forks
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




// Função com processamento do Child
void subvector_process(int start, int stop, int v[], int pipeChildParent[])
{
	//Fechar pipe de leitura
	close(pipeChildParent[0]);
	
	int dim = stop-start+2;
	//printf("%d",dim);
	int* buffer = malloc(sizeof(int)*dim);
	if (buffer == NULL) {
        perror("Erro malloc");
        exit(EXIT_FAILURE); // Saída em caso de falha na alocação de memória
    }
	//Analise dos numeros
	int value;
	int idx = 0;
	for(; start <= stop; start++ )
	{
		value = v[start];
		if(LOWER_VALUE_SUBVECTOR <= value && value <= UPPER_VALUE_SUBVECTOR)
		{
			//printf(".");
			buffer[idx++] = value;
			//write(pipeChildParent[1], &value, sizeof(int));
		}
	}
	
	buffer[idx] = '\0';
	if (write(pipeChildParent[1], buffer, sizeof(int) * dim) == -1) {
        perror("Erro write");
        free(buffer); // Liberar memória alocada
        close(pipeChildParent[1]);
        exit(EXIT_FAILURE);
    }
	free(buffer);
	//Insercao de caracter terminador
	//value = '\0';
	//write(pipeChildParent[1], &value, sizeof(int));
	
	//Fechar pipe de escrita
	//printf("X");
	close(pipeChildParent[1]);
	
	return;
}



//Funcao principal de processamento do array
int vector_get_in_range(int v[], int v_sz, int sv[], int min, int max, int n_processes)
{
	int idx_start=0;
	int idx_stop;
	

	//Criação de pipes
	int (*ptr)[2] = malloc(sizeof(int) * n_processes * 2);
	for(int i=0; i<n_processes;i++)
	{
		//Verificação do estado dos pipes
		if (pipe(ptr[i]) < 0)
		{
			perror("pipeChild");
			free(ptr[0]);
			return -1;
		}
	}

	
	
	//Criação dos processos filho
	int size = v_sz / n_processes;
	int countChild=0;

	for(;countChild < n_processes; countChild++)
	{
		if(countChild < n_processes-1)
			idx_stop = idx_start + size-1;
		else
			idx_stop = v_sz-1;

		
		//Criar processos filho
		if(fork()==0)
		{
			subvector_process(idx_start, idx_stop, v, ptr[countChild]);
			
			//Saida da função durante no processo de um filho
			exit(0);
		}
		
		
		idx_start += size;
	}

	

	//Aguardar fechar todos os processos filho
	for(int i = 0; i <n_processes; i++)
	{
		wait(NULL);
	}
	
		
		
	//Leitura do pipe
	int idx = 0;
	for(int i = 0; i <n_processes; i++)
	{
		int num;
		while(1)
		{
			if (read(ptr[i][0], &num, sizeof(int)) == -1) 
			{
                perror("Erro read");
                close(ptr[i][0]);
                free(ptr); // Liberar memória alocada
                return -1;
            }
			if(num != '\0')
			{
				sv[idx] = num;
				idx ++;
			}
			else
			{
				break;
			}
		}
	}
		
	sv = realloc(sv, idx* sizeof(int));
		

		
	//Fechar os pipes
	for(int i = 0; i < n_processes; i++)
	{
		close(ptr[i][0]);
		close(ptr[i][1]);
	}
	
	free(ptr); // Liberar memória alocada
		
	return idx;
}
