#include "Server.hpp"

Server::Server(const std::string &port, const std::string &password)
{
	_port = std::atoi(port.c_str());
	if (_port <= 0 || _port > 65535)
		throw std::invalid_argument("Invalid port");
	_password = password;
	_initSocket();
}

Server::~Server()
{
	for (std::size_t i = 0; i < _pollFds.size(); ++i)
		close(_pollFds[i].fd);
}

void Server::_initSocket()
{
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0)
		throw std::runtime_error("Failed to create socket");

	int opt = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("Failed to set socket options");
	
	struct sockaddr_in serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(_port);

	if (bind(_serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
		throw std::runtime_error("Failed to bind socket");

	if (listen(_serverSocket, 5) < 0)
		throw std::runtime_error("Failed to listen on socket");

	struct pollfd serverPfd;
	serverPfd.fd = _serverSocket;
	serverPfd.events = POLLIN;
	_pollFds.push_back(serverPfd);
	
	std::cout << "Server listening on port " << _port << std::endl;
}

void Server::run()
{
	while (true)
	{
		int ret = poll(&_pollFds[0], _pollFds.size(), -1);
		if (ret < 0)
			throw std::runtime_error("poll() failed");
		
		for (std::size_t i = 0; i < _pollFds.size(); ++i)
		{
			if (_pollFds[i].revents & POLLIN)
			{
				if (_pollFds[i].fd == _serverSocket)
					_acceptNewClient();
				else
					_handleClientMessage(_pollFds[i].fd);
			}
		}
	}
}

void Server::_acceptNewClient()
{
	int clientFd = accept(_serverSocket, NULL, NULL);
	if (clientFd < 0)
		return;
	
	struct pollfd clientPfd;
	clientPfd.fd = clientFd;
	clientPfd.events = POLLIN;
	_pollFds.push_back(clientPfd);

	Client *newClient = new Client(clientFd);
	_clients[clientFd] = newClient;

	std::cout << "New client connected (fd: " << clientFd << ")" << std::endl;
}

void Server::_handleClientMessage(int clientFd)
{
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));
	int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		std::cout << "Client disconnected (fd: " << clientFd << ")" << std::endl;
		_removeClient(clientFd);
		return;
	}

	std::string msg(buffer);
	std::istringstream ss(msg);
	std::string line;
	while (std::getline(ss, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		_parseCommand(clientFd, line);
	}
}

void Server::_removeClient(int clientFd)
{
	close(clientFd);
	for (std::size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd == clientFd)
		{
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
	std::map<int, Client *>::iterator it = _clients.find(clientFd);
	if (it != _clients.end())
	{
		delete it->second;
		_clients.erase(it);
	}
}

void Server::_parseCommand(int clientFd, const std::string &msg)
{
	Client *client = _clients[clientFd];
	std::istringstream ss(msg);
	std::string cmd;
	ss >> cmd;

	if (cmd == "PASS")
	{
		std::string pass;
		ss >> pass;
		if (pass == _password)
			client->setPass(true);
		else
			_sendMessage(clientFd, "464 :Password incorrect\r\n");
	}
	else if (cmd == "NICK")
	{
		std::string nick;
		ss >> nick;
		if (_nicknameExists(nick))
			_sendMessage(clientFd, "433 * " + nick + " :Nickname is already in use\r\n");
		else
			client->setNickname(nick);
	}
	else if (cmd == "USER")
	{
		std::string username, unused, unused2, realname;
		ss >> username >> unused >> unused2;
		std::getline(ss, realname);
		client->setUsername(username);
	}

	client->tryAuthenticate();
	if (client->isAuthenticated())
		_sendMessage(clientFd, ":ircserv 001 " + client->getNickname() + " : Welcome to ft_irc!\r\n");
}

void Server::_sendMessage(int fd, const std::string &msg)
{
	send(fd, msg.c_str(), msg.length(), 0);
}

bool Server::_nicknameExists(const std::string &nickname)
{
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == nickname)
			return true;
	}
	return false;
}