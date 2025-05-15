#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>

class Server 
{
	private:
		int _port;
		std::string _password;
		int _serverSocket;
		std::vector<struct pollfd> _pollFds;
		std::map<int, Client *> _clients;

		void _initSocket();
		void _acceptNewClient();
		void _handleClientMessage(int clientFd);
		void _removeClient(int clientFd);
		void _parseCommand(int clientFd, const std::string &msg);
		void _sendMessage(int fd, const std::string &msg);
		bool _nicknameExists(const std::string &nickname);

	public:
		Server(const std::string &port, const std::string &password);
		~Server();

		void run();
};

#endif