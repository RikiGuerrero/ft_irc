#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

class Server 
{
	private:
		int _port;
		std::string _password;
		int _serverSocket;

		void _initSocket();

	public:
		Server(const std::string &port, const std::string &password);
		~Server();

		void run();
};

#endif