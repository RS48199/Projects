#include <errno.h>
//#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <pthread.h>

//#include <sys/types.h>
//#include <sys/wait.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
//#include <netdb.h>
#include <sys/un.h>





#include "tcp_connection.h"



//Mensagem de erro
void fatal_tcp(char* msg)
{
	printf("--ERROR--\n");
	printf("%s\n%s\n", msg, strerror(errno));
	printf("---------\n");
	exit(-1);
}



int tcp_socket_server_init(int serverPort)
{
	char* ip = SERVER_ADDR;
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	
	//Código para reutilização do socket
	int flag = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
		fatal_tcp("Erro no setsockopt");
	if(sockfd == -1)
		fatal_tcp("Falha ao criar o socket");
		
	
	struct sockaddr_in serv_addr;
	//Registar endereço local de modo a que os clientes possam contactar
	memset((char*)&serv_addr, 0 , sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(serverPort);
	
	
	//Registo de endereços dos pedidos
	int retBind = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (retBind < 0)
		fatal_tcp("Erro ao efectuar o bind");

	//Aceitação de cliente
	if(listen(sockfd, MAX_CLIENT_CONNECTION) < 0)						//Definição de limite máximo de clientes
		fatal_tcp("Erro no listen");
		

	
	return sockfd;
}


int tcp_socket_server_accept(int serverSocket)
{
	int sockfd = serverSocket;
	

	struct sockaddr_in client;
	socklen_t length = sizeof(client);
	
	//Criação de socket para o cliente
	int newSockfd = accept(sockfd, (struct sockaddr *)(&client), &length);
	
	if(newSockfd < 0)
	{
		close(sockfd);
		fatal_tcp("Erro ao efetuar o accept");
	}	
	
	printf("\nLigação estabelecida");
	
	return newSockfd;

}
