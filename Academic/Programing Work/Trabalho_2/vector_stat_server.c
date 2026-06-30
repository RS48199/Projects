#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>




#include "tcp_connection.h"
#include "unix_connection.h"
#include "vector_stat_proc_threads.h"
#include "vector_stat_proc.h"
#include "vector_stat_seq.h"


#define BUFFER_SIZE (128)

//Estrutura para passagem de dados à thread
typedef struct M_ThreadArguments
	{
		int* sockClient;
		int mode;
	}ThreadArguments;




//Mensagem de erro
void fatal(char* msg)
{
	printf("--ERROR--\n");
	printf("%s\n%s\n", msg, strerror(errno));
	printf("---------\n");
	exit(-1);
}





//Função a executar em cada thread que corresponde a um socket do cliente
void* client_routine(void* arg)
{
	//Tratamento dos argumentos passados
	ThreadArguments* arguments = (ThreadArguments*)arg;
	int client = *(arguments->sockClient);
	int mode = arguments->mode;
	
	
	//Aquisição do tamanho do vector
	int dimVector;
	read(client, &dimVector, sizeof(int));
	
	//Aquisição de faixa de valores a analisar no vector
	int minValue;
	read(client, &minValue, sizeof(int));
	int maxValue;
	read(client, &maxValue, sizeof(int));
	
	//Aquisição de numero de threads/processos para analisar o vector 
	int n_process;
	read(client, &n_process, sizeof(int));
	
	//Construção de vector recebido
	int* receivedVector = malloc(sizeof(int)*dimVector);
	if(receivedVector == NULL)
	{
		fatal("ERRO malloc vector");
		free(arguments);
        exit(EXIT_FAILURE);
	}
	
	
    if (read(client, receivedVector, sizeof(int)*dimVector) == -1) 
    {
        fatal("ERRO read");
        free(receivedVector);
		free(arguments);
        exit(EXIT_FAILURE);
    }
    /*
    for(int i=0; i<dimVector;i++)
		printf("\t%d",receivedVector[i]);
*/
	//Alocação de subvector para guardar numeros na faixa
	int* subVector = malloc(sizeof(int) * dimVector);			//Posterior realloc do tamanho do subvector
    if(subVector == NULL)
    {
		fatal("ERRO malloc subvector");
        free(receivedVector);
		free(arguments);
        exit(EXIT_FAILURE);
	}
    
    //Inicio de contagem de tempo de execução
	struct timeval t1,t2;
    gettimeofday(&t1, NULL);
    
    //Processamento de números com threads ou processos
    int dimSubVector;
    if(mode == 0)
    {
		dimSubVector =  vector_get_in_range_with_threads(receivedVector, dimVector, subVector, minValue, maxValue, n_process);
    }
    else
	{
		dimSubVector =  vector_get_in_range(receivedVector, dimVector, subVector, minValue, maxValue, n_process);
	}
	
	//FIM do tempo de execução do programa
    gettimeofday(&t2, NULL);
    long elapsed = ((long)t2.tv_sec - t1.tv_sec) * 1000000L + (t2.tv_usec - t1.tv_usec);
    long sec = elapsed / (long)1e6;
    long aux = elapsed % (long)1e6;
    long mil = aux / (long)1e3;
    long mic = aux % (long)1e3;
	
	//Redimensionamento de subVector para tamanho adequado
	subVector = realloc(subVector, dimSubVector * sizeof(int));
	
	
	//Apresentacao ao utilizador
	printf("\tRecebido vector de %d elementos\n",dimVector);
	printf("\tEncontrados valores entre %d e %d\n", LOWER_VALUE_SUBVECTOR, UPPER_VALUE_SUBVECTOR);
	printf("\tRetornado subvector com %d elementos\n", dimSubVector);
	printf ("\tTempo de processamento = %lds%ld,%ldms\n", sec, mil, mic);
	
	//Envio de subVector processado
	write(client, &dimSubVector, sizeof(int) );
	write(client, subVector, dimSubVector*sizeof(int) );
	

	//printf("\nServer routine exitting\n");
	free(receivedVector);
	free(subVector);
		
	//Liberação da memória alocada para passagem de argumentos
	free(arguments);
	
	//Fecho da ligação do cliente
	close(client);
		
		
	return NULL;
} 



//Inicialização do socket e aceitação das ligacões aos clientes
void* un_process(void* value)
{
	//Opção de modo para tratamento de vector de números	0->por threads	1->por processos
	int option = *((int*)value);
	
	//Criar socket
	int sockfd;
	sockfd= un_socket_server_init(UNIX_SOCKET_PATH);
	
	
	while(1)
	{
		//Aceitação de conexão com cliente
		int client;
		client= un_socket_server_accept(sockfd);
		if(client < 0)
			fatal("Cliente rejeitado");
		
		printf(" - Cliente (%d)\n",client);

		//Criação de thread do cliente e passagem de argumentos à mesma
		pthread_t thread;
		ThreadArguments* arg = malloc(sizeof(ThreadArguments));
		arg->mode = option;
		arg->sockClient = &client;
		int un_pthread = pthread_create(&thread, NULL, client_routine, (void *)arg);
		if(un_pthread < 0)
		{
			printf("ERROR thread UNIX\n");
			free(arg);
			return NULL;
		}
	}
	
	//Fecho do socket
	close(sockfd);
	printf("\nFecho do servidor");
	
	
	return 0;
}






void* tcp_process(void* value)
{
	//Opção de modo para tratamento de vector de números	0->por threads	1->por processos
	int option = *((int*)value);
	
	int port = SERVER_PORT;
	
	//Criação de socketTCP
	int sockfd;
	sockfd = tcp_socket_server_init(port);
	
	
	while(1)
	{
		//Aceitação de conexão com cliente
		int client;
		client = tcp_socket_server_accept(sockfd);
		if(client < 0)
			fatal("Cliente rejeitado");
		
		
		printf(" - Cliente (%d)\n",client);
		
		
		
		//Criação de thread do cliente e passagem de argumentos à mesma
		pthread_t thread;
		ThreadArguments* arg = malloc(sizeof(ThreadArguments));
		arg->mode = option;
		arg->sockClient = &client;
		int un_pthread = pthread_create(&thread, NULL, client_routine, (void *)arg);
		if(un_pthread < 0)
		{
			printf("ERROR thread UNIX\n");
			return NULL;
		} 
	}
			
	//Fecho do socket
	close(sockfd);
	printf("\nFecho do servidor");
	
	return 0;
}




int main(int argc, char** argv)
{
	
	if(argc != 2)
    {
        printf("Utilização incorreta. Forma correta: ./vector_stat_server <suporte de processamento>\n");
        return -1;
    }
    
    //Determinação do tipo de processamento de dados
    char caracter [2];
    int option = -1;
	strcpy(caracter,argv[1]);
	
	printf("Tratamento de dados através de ");
    switch(caracter[1])
    {
		case 'p':
			printf("multiplos processos.\n");
			option = 1;
			break;
		case 't':
			printf("multiplas threads.\n");
			option = 0;
			break;
		default:
			printf("... -> Modo de tratamento desconhecido\n");
			return -1;
	}	
	
	
	//Criação de duas threads para tratamento de comunicação unix e tcp em simultâneo
	pthread_t* main_ths = malloc(2* sizeof(pthread_t));
	
	
	int un_res_pthread = pthread_create( &main_ths[0], NULL, un_process, &option);
	if(un_res_pthread < 0)
	{
		printf("ERROR theread UNIX\n");
		free(main_ths);
		return -1;
	}
	
	int tcp_res_pthread = pthread_create( &main_ths[0], NULL, tcp_process, &option);
	if(tcp_res_pthread < 0)
	{
		printf("ERROR theread TCP\n");
		free(main_ths);
		return -1;
	}


	//Conclusão das threads
	void* ret;
	pthread_join(main_ths[0], &ret);
	printf("Thread de comunicação UNIX fechada (ID:%ld)\n", main_ths[0]);
	pthread_join(main_ths[1], &ret);
	printf("Thread de comunicação TCP fechada (ID:%ld)\n", main_ths[1]);
	
	
	//Libertação de memória associada às threads
	free(main_ths);
	
	return 0;
}
