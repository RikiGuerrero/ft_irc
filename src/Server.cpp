#include "Server.hpp"

Server::Server(const std::string &port, const std::string &password)
{//check parameters in the constructor
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
}

void Server::_initSocket()
{
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);//cria o socket
	if (_serverSocket < 0)
		throw std::runtime_error("Failed to create socket");

	int opt = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)//altera o comportamento do socker para reutilizar endereços locais (Padrao)
		throw std::runtime_error("Failed to set socket options");
	
	struct sockaddr_in serverAddr;//estrutura para representar endereços IPv4
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;//IPv4
	serverAddr.sin_addr.s_addr = INADDR_ANY;//aceita conexoes em qqlt interface
	serverAddr.sin_port = htons(_port);//puerto

	if (bind(_serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)//associa um socket a um endreço e uma porta especificas, permite ouvir e aceitar conexao nessa porta
		throw std::runtime_error("Failed to bind socket");

	if (listen(_serverSocket, 5) < 0)//coloca um socket em modo de escuta, 
		throw std::runtime_error("Failed to listen on socket");

	struct pollfd serverPfd;//usada para monitorar multiplos servidores de arquivo
	serverPfd.fd = _serverSocket;//nesse caso do servidor
	serverPfd.events = POLLIN;
	_pollFds.push_back(serverPfd);//adc ao vector de fds
	
	std::cout << "Server listening on port " << _port << std::endl;
}

void Server::run()
{
	while (true)
	{
		int ret = poll(&_pollFds[0], _pollFds.size(), -1);//monitora multiplos fds
		if (ret < 0)
			throw std::runtime_error("poll() failed");
		
		for (std::size_t i = 0; i < _pollFds.size(); ++i)
		{
			if (_pollFds[i].revents & POLLIN)//checka se há dados disponiveis para leitura em todos os fds
			{
				if (_pollFds[i].fd == _serverSocket)//se o fd for igual do server é para se conectar 
					_acceptNewClient();
				else
					_handleClientMessage(_pollFds[i].fd);//se nao é mensagem
			}
		}
	}
}

void Server::_acceptNewClient()
{
	int clientFd = accept(_serverSocket, NULL, NULL);//aceita a conexao do cliente em um socket que esta em modo de escuta
	if (clientFd < 0)//devolve um novo fd que representa essa conexao
		return;
	
	struct pollfd clientPfd;//escuta esse fd
	clientPfd.fd = clientFd;
	clientPfd.events = POLLIN;
	_pollFds.push_back(clientPfd);//adc no vector de fds

	Client *newClient = new Client(clientFd);//Inicia um objeto desse cluente
	_clients[clientFd] = newClient;//adiciona ao map, associados pelo fd	

	std::cout << "New client connected (fd: " << clientFd << ")" << std::endl;
}

void Server::_handleClientMessage(int clientFd)
{
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));
	int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);//receber dados de um socket conectado
	if (bytesRead <= 0)//si é == 0, o cliente se desconectoue -1 se falha
	{
		std::cout << "Client disconnected (fd: " << clientFd << ")" << std::endl;
		_removeClient(clientFd);
		return;
	}

	std::string msg(buffer);//guarda todo o texto recebido
	std::istringstream ss(msg);
	std::string line;
	while (std::getline(ss, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')//remove retorno de carro
			line.erase(line.size() - 1);
		_parseCommand(clientFd, line);
	}
}

void Server::_removeClient(int clientFd)
{
	close(clientFd);//fecha o fd
	for (std::size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd == clientFd) //apaga do vector de fds
		{
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
	std::map<int, Client *>::iterator it = _clients.find(clientFd);
	if (it != _clients.end())//apaga o objeto
	{
		delete it->second;
		_clients.erase(it);
	}
}
