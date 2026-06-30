


#ifndef __UNIX_CONNECTION_H__
#define __UNIX_CONNECTION_H__


	#define UNIX_SOCKET_PATH   "/home/aluno/Desktop/my_socket_path"
	#define MAX_CLIENT_CONNECTION 5


	void fatal_un(char* msg);
	int un_socket_server_init(const char *serverEndPoint);
	int un_socket_server_accept(int serverSocket);

#endif
