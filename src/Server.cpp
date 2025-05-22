#include "Server.hpp"

extern volatile sig_atomic_t g_signalStatus;

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
	{
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

	std::cout << "Client disconnected and removed. Socket fd: " << clientFd << std::endl;
}
