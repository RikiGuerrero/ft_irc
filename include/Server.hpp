#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>

class Server 
{
	private:
		int _port;
		std::string _password;
		int _serverSocket;
		std::vector<struct pollfd> _pollFds;

		void _initSocket();
		void _acceptNewClient();
		void _handleClientMessage(int clientFd);
		void _removeClient(int clientFd);

	public:
		Server(const std::string &port, const std::string &password);
		~Server();

		void run();
};

#endif