#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channel.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include <arpa/inet.h>
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
		void _handleClientWrite(int clientFd);
		void _sendWelcomeMessage(int clientFd);
		void _broadcastToChannel(const std::string &channel, const std::string &msg, int excludeFd = -1);
		bool _nicknameExists(const std::string &nickname);
		void _pass(Client *client, int clientFd, const std::string &msg);
		void _nick(Client *client, int clientFd, const std::string &msg);
		void _user(Client *client, int clientFd, const std::string &msg);
		void _join(Client *client, int clientFd, const std::string &msg);
		void _acceptClient(Channel *channel, Client *client, int clientFd, const std::string &channelName);
		void _ping(int clientFd, const std::string &msg);
		void _part(Client *client, int clientFd, const std::string &msg);
		void _privmsg(Client *sender, int clientFd, const std::string &msg);
		void _quit(Client *client, int clientFd, const std::string &msg);

		void _invite(Client *client, int clientFd, const std::string &msg);
		void _kick(Client *client, int clientFd, const std::string &msg);
		void _topic(Client *client, int clientFd, const std::string &msg);
		void _mode(Client *client, int clientFd, const std::string &msg);
		void _modeO(Client *client, int clientFd, std::string &flag, std::string &user, Channel *channel);
		void _modeI(Channel *channel, const std::string &flag);
		void _modeT(Channel *channel, const std::string &flag);
		void _modeL(Client *client, Channel *channel, const std::string &flag, const std::string &parameters);
		void _modeK(Client *client, Channel *channel, const std::string &flag, const std::string &parameters);
		
	public:
		Server(const std::string &port, const std::string &password);
		~Server();

		void run();
};

#endif