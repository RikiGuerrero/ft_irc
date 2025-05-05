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
	close(_serverSocket);
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
	
	std::cout << "Server listening on port " << _port << std::endl;
}

void Server::run()
{
	while (true)
	{
		int clientSocket = accept(_serverSocket, NULL, NULL);
		if (clientSocket >= 0)
		{
			std::cout << "New connection accepted (fd: " << clientSocket << ")" << std::endl;
			close(clientSocket);
		}
	}
}