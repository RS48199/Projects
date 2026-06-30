#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#include "tcp_connection.h"
#include "unix_connection.h"
#include "vector_stat_seq.h"
#include "vector_stat_proc_threads.h"



#define BUFFER_SIZE (128)



//Estrutura para passagem de dados à thread
typedef struct M_Arguments{
	int* sockClient;
	int mode;
}Arguments;







//Mensagem de erro
void fatal(char* msg)
{
	printf("--ERROR--\n");
	printf("%s\n%s\n", msg, strerror(errno));
	printf("---------\n");
	exit(-1);
}



//Função de geração de dados e troca de informação com o servidor
int vector_proc(int sockfd, long dimensaoVector, int numeroProc)
{


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
		free(vector);
		return -1;
	}
	

	
	//Preenchimento aleatório do vector
	vector_init_rand(vector, dimensaoVector, LOWER_LIMIT, UPPER_LIMIT);

	
	
    int n_elements;
    
    //Elementos enviados ao servidor indicações de nº de elementos e forma de tratamento do vector
	int extraElements = (sizeof(n_elements)/4)+(sizeof(LOWER_VALUE_SUBVECTOR)/4)+(sizeof(UPPER_VALUE_SUBVECTOR)/4)+(sizeof(numeroProc)/4);
	
	//Número de elementos passado ao servidor
    n_elements = dimensaoVector + extraElements;
    
    //Buffer para envio de elementos
    int* buffer = malloc(n_elements*sizeof(int));
    
    //Envio de váriaveis de apoio
    int idx = 0;
    buffer[idx++] = dimensaoVector;				//Número de elementos enviado
    buffer[idx++] = LOWER_VALUE_SUBVECTOR;		//Valor minimo de elemento no vector
    buffer[idx++] = UPPER_VALUE_SUBVECTOR;		//Valor maximo de elemento no vector
    buffer[idx++] = numeroProc;					//Valor de Processos/Threads para analisar o vector
    
	
	//Passagem de elementos para o buffer
    int count = 0;
    while(count < dimensaoVector)
    {
		buffer[idx++] = vector[count++];
		
	}

	//Envio do buffer ao servidor
	write(sockfd, buffer, n_elements*sizeof(int) );
	
	
	
	//Leitura do subvector retornado pelo servidor
	int size;
	read(sockfd, &size, sizeof(int));
	
	int received_array[size];
	if (read(sockfd, received_array, sizeof(received_array)) == -1) 
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

	printf("\tEnviado vector de %ld elementos\n",dimensaoVector);
	printf("\tProcessar valores entre %d e %d\n", LOWER_VALUE_SUBVECTOR, UPPER_VALUE_SUBVECTOR);
    printf("\tRecebido subvector de %d elementos\n", size);


	//Liberação de memória alocada
	free(buffer);
    free(vector);
    free(subvector);
    

    return 0;
}




//Inicialização de comunicação do cliente em UNIX
int un_socket_client_init(const char* serverEndPoint, long dimensaoVector, int numeroProc)
{
	//Criação de socket UNIX
	int sockfd;
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0 )
		fatal("Erro ao pedir o descritor");
		
	struct sockaddr_un serv_addr;
	
	//Registar endereço local de modo a que os servidores possam contactar
	memset((char*)&serv_addr, 0 , sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, serverEndPoint);
	
	
	printf("O cliente vai ligar-se ao servidor no socket %s\n", serverEndPoint);

	//Conexão ao socket
	int connection = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if ( connection < 0 )
        fatal("Falha no connect");
	
	printf("\nLigacao estabelecida com o servidor\n");
	

	//Geração de dados e troca de informação com o servidor
	vector_proc(sockfd, dimensaoVector, numeroProc);
	
	//Fecho do socket
	close(sockfd);
	return 0;
}



//Inicialização de comunicação do cliente em TCP
int tcp_socket_client_init(const char* host, int port, long dimensaoVector, int numeroProc)
{
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0 )
		fatal("Erro ao pedir o descritor");
		
	struct sockaddr_in serv_addr;
	//Registar endereço local de modo a que os clientes possam contactar
	memset((char*)&serv_addr, 0 , sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(host);
	serv_addr.sin_port = htons(port);
	
	
	printf("O cliente vai ligar-se ao servidor no socket pelo IP %s\n", host);


	//Conexão ao socket
	int connection = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if ( connection < 0 )
        fatal("Falha no connect");
	
	printf("\nLigacao estabelecida com o servidor\n");

	//Geração de dados e troca de informação com o servidor
	vector_proc(sockfd, dimensaoVector, numeroProc);

	//Fecho do socket
	close(sockfd);
	
	return 0;
}



int main(int argc,char** argv)
{
	//Tratamento de dados inseridos por console
	if(argc != 4)
    {
        printf("Utilização incorreta. Forma correta: ./vector_stat_client <tipo de conexao> <n elem vector> <tipo processamento>\n");
        return -1;
    }
    
	char option [2];
	strcpy(option,argv[1]);
	
	long dimensaoVector = atol(argv[2]);					//Dimensão do vector passado por stdin		=> v_sz
    int numeroProc=atoi(argv[3]);  					//Numero de processos passado por stdin		=> n_processes
	
	
	//Escolha e adequação ao tipo de ligação
	switch(option[1])
    {
		case 'u':
			printf("Conexao UNIX\n");
			un_socket_client_init(UNIX_SOCKET_PATH, dimensaoVector, numeroProc);
			break;
		case 't':
			printf("Conexao TCP\n");
			int port = SERVER_PORT;
			char* host = SERVER_ADDR;
			tcp_socket_client_init(host, port, dimensaoVector, numeroProc);
			break;
		default:
			printf("Modo desconhecido\n");
			return -1;
	}	

	return 0;
}


