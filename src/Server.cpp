#include "Server.hpp"

extern volatile sig_atomic_t g_signalStatus;

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

void Server::_parseCommand(int clientFd, const std::string &msg)
{
	Client *client = _clients[clientFd];
	std::istringstream ss(msg);
	std::string cmd;
	ss >> cmd;

	bool wasAuthenticated = client->isAuthenticated();

	if (!wasAuthenticated)//si no esta autenticado
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
		std::cout << msg << std::endl;
		if (cmd == "JOIN" || cmd == "join")
			_join(client, clientFd, msg);//entra en el canal, falta modo invitacion
		else if (cmd == "PRIVMSG" || cmd == "privvmsg")
			_privmsg(client, clientFd, msg);//envia un mensaje privado o a un canal
		else if (cmd == "PING" || cmd == "ping")
			_ping(clientFd, msg);
		else if (cmd == "PART" || cmd  == "part")
			_part(client, clientFd, msg);//excluir un usuario de un canal
		else if (cmd == "TOPIC" || cmd == "topic")
			_topic(client, clientFd, msg);
		else if (cmd == "INVITE" || cmd == "invite")
			_invite(client, clientFd, msg);
		else if (cmd == "KICK" || cmd == "kick")
			_kick (client, clientFd, msg);
		else if (cmd == "MODE" || cmd == "mode")
			_mode(client, clientFd, msg);
		else if (cmd == "QUIT")
			_quit(client, clientFd, msg);
	}

	client->tryAuthenticate();//autentica el cliente

	if (!wasAuthenticated && client->isAuthenticated())
		_sendWelcomeMessage(clientFd);
}

void Server::_initSocket()
{
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);//crea o socket
	if (_serverSocket < 0)
		throw std::runtime_error("Failed to create socket");

	int opt = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)//cambia elcorpontamiento del socket para reutilizar direcciones locales (Patron)
	{
		close(_serverSocket);
		throw std::runtime_error("Failed to set socket options");
	}
	
	if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) < 0)
	{
		close(_serverSocket);
		throw std::runtime_error("Failed to set socket to non-blocking");
	}

	struct sockaddr_in serverAddr;//estructura para representar direcciones IPv4
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;//IPv4
	serverAddr.sin_addr.s_addr = INADDR_ANY;//acepta conecciones en cualquier interfaz
	serverAddr.sin_port = htons(_port);//puerto

	if (bind(_serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)//asocia un socket a una dirección y un puerto especifico, permite oyer y aceptar coneccion en ese puerto
	{
		close(_serverSocket);
		throw std::runtime_error("Failed to bind socket");
	}

	if (listen(_serverSocket, 5) < 0)//pone un socket en modo de escuta, 
	{
		close(_serverSocket);
		throw std::runtime_error("Failed to listen on socket");
	}

	struct pollfd serverPfd;//usada para monitorar multiplos fds
	serverPfd.fd = _serverSocket;//en ese caso del servidor
	serverPfd.events = POLLIN;
	_pollFds.push_back(serverPfd);//adc al vector de fds
	
	std::cout << "Server listening on port " << _port << std::endl;
}

void Server::run()
{
	while (g_signalStatus)
	{
		int ret = poll(&_pollFds[0], _pollFds.size(), -1);//monitora multiplos fds
		if (ret < 0)
		{
			if (errno != EINTR)
				throw std::runtime_error("poll() failed");
			continue;
		}
		for (std::size_t i = 0; i < _pollFds.size(); ++i)
		{
			if (_pollFds[i].revents & POLLIN)//checkea si hay dados disponibles para lectura en todos los fds
			{
				if (_pollFds[i].fd == _serverSocket)//si el fd es igual al del server es para  conectarse
					_acceptNewClient();
				else
					_handleClientMessage(_pollFds[i].fd);//si no es mensaje
			}
		}
	}
}

void Server::_acceptNewClient()
{
	sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	int clientFd = accept(_serverSocket, (struct sockaddr *)&clientAddr, &addrLen);//acepta la coneccion del cliente en un socket que esta en modo de escucha
	if (clientFd < 0)//retorna un nuevo fd que representa esa coneccion
		return;
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(clientFd);
		throw std::runtime_error("Failed to set client socket to non-blocking");
	}
	struct pollfd clientPfd;//escuchaa ese fd
	clientPfd.fd = clientFd;
	clientPfd.events = POLLIN;
	_pollFds.push_back(clientPfd);//adc en el vector de fds

	char clientIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
	std::string hostname = clientIp;

	Client *newClient = new Client(clientFd, hostname);//Inicia un objeto del cliente
	_clients[clientFd] = newClient;//adiciona al map, asociados por el fd	

	std::cout << "New client connected (fd: " << clientFd << ")" << std::endl;
}

void Server::_handleClientMessage(int clientFd)
{
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));
	int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);//reciber datos de un socket conectado
	if (bytesRead <= 0)//si es == 0, el cliente se desconecto y es -1 se falha
		return _removeClient(clientFd);
	
	Client *client = _clients[clientFd];
	client->appendToBuffer(buffer);

	std::string &buf = client->getRecvBuffer();
	std::size_t pos;

	while ((pos = buf.find('\n')) != std::string::npos) //busca el primer \n
	{
		std::string msg = buf.substr(0, pos); //corta la cadena hasta el \n
		if (!msg.empty() && msg[msg.size() - 1] == '\r') //si el ultimo caracter es \r, lo elimina
			msg.erase(msg.size() - 1);
		_parseCommand(clientFd, msg); //analiza el comando
		buf.erase(0, pos + 1); //borra el mensaje procesado del buffer
	}
}

void Server::_removeClient(int clientFd)
{
	close(clientFd);//cierra o fd
	for (std::size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd == clientFd) //apaga del vector de fds
		{
			_pollFds.erase(_pollFds.begin() + i);
			break;
		}
	}
	std::map<int, Client *>::iterator it = _clients.find(clientFd);
	if (it != _clients.end())//apaga el objeto
	{
		delete it->second;
		_clients.erase(it);
	}

	std::cout << "Client disconnected and removed. Socket fd: " << clientFd << std::endl;
}
