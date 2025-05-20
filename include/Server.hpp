#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channel.hpp"
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
		int _serverSocket;
		std::string _password;
		std::vector<struct pollfd> _pollFds;
		std::map<int, Client *> _clients;
		std::map<std::string, Channel *> _channels;

		void _initSocket();
		void _acceptNewClient();
		void _handleClientMessage(int clientFd);
		void _removeClient(int clientFd);
		void _parseCommand(int clientFd, const std::string &msg);
		void _sendMessage(int fd, const std::string &msg);
		void _handleNick(int clientFd, const std::string &nickname);
		void _handleUser(int clientFd, const std::string &username);
		void _handleJoin(int clientFd, const std::string &channelName);
		void _handlePrivmsg(int clientFd, const std::string &target, const std::string &message);
		void _handlePart(int clientFd, const std::string &channelName, const std::string &reason);
		void _handleQuit(int clientFd, const std::string &reason);
		void _sendWelcomeMessage(int clientFd);
		void _broadcastToChannel(const std::string &channel, const std::string &msg, int excludeFd = -1);
	
		bool _nicknameExists(const std::string &nickname);

	public:
		Server(const std::string &port, const std::string &password);
		~Server();

		void run();
};

#endif