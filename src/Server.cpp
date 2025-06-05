#include "Server.hpp"

extern volatile sig_atomic_t g_signalStatus;

Server::Server(const std::string &port, const std::string &password)
{
	for (std::string::const_iterator it = port.begin(); it != port.end(); *it++)
	{
		if (!isdigit(*it))
			throw std::invalid_argument("Invalid port");
	}
	_password = password;
	_port = std::atoi(port.c_str()); 
	if (_port <= 1024 || _port > 65535)
		throw std::invalid_argument("Invalid port");

	_initSocket();
}

Server::~Server()
{
	for (std::size_t i = 0; i < _pollFds.size(); ++i)
		close(_pollFds[i].fd);
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		delete it->second;
	_clients.clear();
	for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;
	_channels.clear();
}

void Server::_parseCommand(int clientFd, const std::string &msg)
{
	Client *client = _clients[clientFd];
	std::istringstream ss(msg);
	std::string cmd;
	ss >> cmd;

	
	bool wasAuthenticated = client->isAuthenticated();
	
	if (!wasAuthenticated)
	{
		if (cmd == "PASS" || cmd == "pass")
			_pass(client, clientFd, msg);
		else if (cmd == "NICK" || cmd == "nick")
			_nick(client, clientFd, msg);
		else if (cmd == "USER" || cmd == "user")
			_user(client, clientFd, msg);
		}
	else
	{
		if (cmd == "JOIN" || cmd == "join")
			_join(client, clientFd, msg);
		else if (cmd == "PRIVMSG" || cmd == "privmsg")
			_privmsg(client, clientFd, msg);
		else if (cmd == "PING" || cmd == "ping")
			_ping(clientFd, msg);
		else if (cmd == "PART" || cmd  == "part")
			_part(client, clientFd, msg);
		else if (cmd == "TOPIC" || cmd == "topic")
			_topic(client, clientFd, msg);
		else if (cmd == "INVITE" || cmd == "invite")
			_invite(client, clientFd, msg);
		else if (cmd == "KICK" || cmd == "kick")
			_kick (client, clientFd, msg);
		else if (cmd == "MODE" || cmd == "mode")
			_mode(client, clientFd, msg);
		else if (cmd == "QUIT" || cmd == "quit")
			_quit(client, clientFd, msg);
		}
		
		if (_clients.find(clientFd) == _clients.end())
			return ;
		
		client->tryAuthenticate();
		
		if (!wasAuthenticated && client->isAuthenticated())
		_sendWelcomeMessage(clientFd);
	}
	
void Server::_initSocket()
{
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0)
		throw std::runtime_error("Failed to create socket");

	int opt = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		close(_serverSocket);
		throw std::runtime_error("Failed to set socket options");
	}
	
	if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) < 0)
	{
		close(_serverSocket);
		throw std::runtime_error("Failed to set socket to non-blocking");
	}

	struct sockaddr_in serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(_port);

	if (bind(_serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		close(_serverSocket);
		throw std::runtime_error("Failed to bind socket");
	}

	if (listen(_serverSocket, 5) < 0)
	{
		close(_serverSocket);
		throw std::runtime_error("Failed to listen on socket");
	}

	struct pollfd serverPfd;
	serverPfd.fd = _serverSocket;
	serverPfd.events = POLLIN;
	_pollFds.push_back(serverPfd);
	
	std::cout << "Server listening on port " << _port << std::endl;
}

void Server::run()
{
	while (g_signalStatus)
	{
		int ret = poll(&_pollFds[0], _pollFds.size(), -1);
		if (ret < 0)
		{
			if (errno != EINTR)
				throw std::runtime_error("poll() failed");
			continue;
		}
		for (std::size_t i = 0; i < _pollFds.size(); ++i)
		{
			if (_pollFds[i].fd == -1)
				continue;
			if (_pollFds[i].revents & POLLIN)
			{
				if (_pollFds[i].fd == _serverSocket)
					_acceptNewClient();
				else
					_handleClientMessage(_pollFds[i].fd);
			}
			if (_pollFds[i].revents & POLLOUT)
			{
				std::map<int, Client *>::iterator it = _clients.find(_pollFds[i].fd);
				if (it != _clients.end() && !it->second->getSendBuffer().empty())
					_handleClientWrite(_pollFds[i].fd);
			}
		}
	}
}

void Server::_acceptNewClient()
{
	sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int clientFd = accept(_serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
	if (clientFd < 0)
		return;
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(clientFd);
		throw std::runtime_error("Failed to set client socket to non-blocking");
	}
	struct pollfd clientPfd;
	clientPfd.fd = clientFd;
	clientPfd.events = POLLIN;
	clientPfd.revents = 0;
	_pollFds.push_back(clientPfd);

	char clientIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
	std::string hostname = clientIp;

	Client *newClient = new Client(clientFd, hostname);
	_clients[clientFd] = newClient;

	std::cout << "New client connected (fd: " << clientFd << ")" << std::endl;
}

void Server::_handleClientMessage(int clientFd)
{
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));
	int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
		return _removeClient(clientFd);
	
	Client *client = _clients[clientFd];
	client->appendToBuffer(buffer);

	std::string &buf = client->getRecvBuffer();
	std::size_t pos;

	while ((pos = buf.find('\n')) != std::string::npos)
	{
		std::string msg = buf.substr(0, pos);
		if (!msg.empty() && msg[msg.size() - 1] == '\r')
			msg.erase(msg.size() - 1);
		_parseCommand(clientFd, msg);
		if (_clients.find(clientFd) == _clients.end())
			return;
		buf.erase(0, pos + 1);
	}
}

void Server::_removeClient(int clientFd)
{
	for (std::size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd == clientFd)
		{
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
	close(clientFd);
	std::map<int, Client *>::iterator it = _clients.find(clientFd);
	if (it != _clients.end())
	{
		delete it->second;
		_clients.erase(it);
	}

	std::cout << "Client disconnected and removed. Socket fd: " << clientFd << std::endl;
}
