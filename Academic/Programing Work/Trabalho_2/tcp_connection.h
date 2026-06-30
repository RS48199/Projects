

#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__


	#define SERVER_ADDR ("127.0.0.1")
	#define SERVER_PORT (1337)

	#define MAX_CLIENT_CONNECTION 5

	void fatal_tcp(char* msg);
	int tcp_socket_server_init(int serverPort);
	int tcp_socket_server_accept(int serverSocket);

#endif 
