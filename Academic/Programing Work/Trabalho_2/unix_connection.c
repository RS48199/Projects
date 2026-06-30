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



#include "unix_connection.h"





//Mensagem de erro
void fatal_un(char* msg)
{
	printf("--ERROR--\n");
	printf("%s\n%s\n", msg, strerror(errno));
	printf("---------\n");
	exit(-1);
}


int un_socket_server_init(const char *serverEndPoint)
{
	//Remoção de ficheiro especial do socket
	unlink(UNIX_SOCKET_PATH);
	
	
	int sockfd;
	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd < 0 )
		fatal_un("Erro ao pedir o descritor");
	
	struct sockaddr_un serv_addr;
	//Registar endereço local de modo a que os clientes possam contactar
	memset((char*)&serv_addr, 0 , sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, serverEndPoint);
	
	//Registo de endereços dos pedidos
	int retBind = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (retBind < 0)
		fatal_un("Erro ao efectuar o bind");
	
	
	return sockfd;
}

int un_socket_server_accept(int serverSocket)
{
	int sockfd = serverSocket;
	//Aceitação de cliente
	if(listen(sockfd, MAX_CLIENT_CONNECTION) < 0)						//Definição de limite máximo de clientes
		fatal_un("Erro no listen");
		
	struct sockaddr_in client;
	socklen_t length = sizeof(client);
	
	//Criação de socket para o cliente
	int newSockfd = accept(sockfd, (struct sockaddr *)(&client), &length);
	if(newSockfd < 0)
		fatal_un("Erro ao efetuar o accept");
		
	
	printf("\nLigação estabelecida");
	
	return newSockfd;
}
