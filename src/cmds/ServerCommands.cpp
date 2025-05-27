#include "Server.hpp"

//posso passar ss como ponteiro.
void Server::_pass(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string pass, cmd; 
	ss >> cmd >> pass;
	if (pass == _password)
	{
		client->setPass(true);
		std::cout << "Pass is correct!\n";
	}
	else
		_sendMessage(clientFd, "464 :Password incorrect\r\n");//envia mensaje al servidor
}

void Server::_nick(Client *client, int clientFd, const std::string &msg)
{
	std::string nick, cmd;
	std::istringstream ss(msg);
	ss >> cmd;
	if (!client->isPassSet())
		return _sendMessage(clientFd, "451 :You have not registered\r\n");
	ss >> nick;
	if (_nicknameExists(nick))//si no es repetido
		_sendMessage(clientFd, "433 * " + nick + " :Nickname is already in use\r\n");
	else
		client->setNickname(nick);
	std::cout << "Nick: " << client->getNickname() << " is setted!\n"; 
}

void Server::_user(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string username, unused, unused2, realname, cmd;
	//los dos parametros unused son opcionales, hay que gestionar eso
	if (!client->isPassSet())
		return _sendMessage(clientFd, "451 :You have not registered\r\n");
	if (client->isRegistered())
		return _sendMessage(clientFd, "462 :You may not reregister\r\n");
	ss >> cmd >> username >> unused >> unused2;
	std::getline(ss, realname);
	if (username.empty() || unused.empty() || unused2.empty() || realname.empty())
		return _sendMessage(clientFd, "461 USER :Not enough parameters\r\n");	
	if (!realname.empty() && realname[0] == ' ')
		realname = realname.substr(1);
	if (!realname.empty() && realname[0] == ':')
		realname = realname.substr(1);
	client->setUsername(username);
	client->setRealname(realname);
	std::cout << "User: " << client->getUsername() << " is setted!\n"; 
}

void Server::_join(Client *client, int clientFd, const std::string &msg)
{
	std::string cmd, channelName;
	std::istringstream ss(msg);
	ss >> cmd >> channelName;

	if (_channels.find(channelName) == _channels.end())// si el canal no existe
		_channels[channelName] = new Channel(channelName);//crea o canal
	
	Channel *channel = _channels[channelName];

	if (channel->hasClient(client))//si el cliente esta en el canal
		return;
	
	channel->addClient(client);//si no adiciona al set de los clientes

	if (channel->isOperator(client) == false && channel->getTopic().empty())//si el cliente no es operador y el topico esta vacio
		channel->addOperator(client);//adc al set de operadores
	
	_sendMessage(clientFd, ":" + client->getNickname() + " JOIN :" + channelName + "\r\n");

	if (channel->getTopic().empty())//set topic
		_sendMessage(clientFd, ":ircserv 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n");
	else
		_sendMessage(clientFd, ":ircserv 332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
	
	std::string names = ":ircserv 353 " + client->getNickname() + " = " + channelName + " :";
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (channel->hasClient(it->second))
		{
			if (channel->isOperator(it->second))
				names += "@";
			names += it->second->getNickname() + " ";
		}
	}
	names += "\r\n";
	_sendMessage(clientFd, names);

	_sendMessage(clientFd, ":ircserv 366 " + client->getNickname() + " " + channelName + " :End of NAMES list\r\n");

	_broadcastToChannel(channelName, client->getPrefix() + " JOIN :" + channelName + "\r\n", clientFd);//transmissao ao canal
}


void Server::_privmsg(Client *sender, int clientFd, const std::string &msg)
{
	std::string cmd, target, message;
	std::istringstream ss(msg);
	std::string prefix = ":" + sender->getNickname() + "!" + sender->getUsername() + "@localhost PRIVMSG ";

	ss >> cmd >> target >> message;
	std::getline(ss, message);
	if (message[0] == ':')
		message = message.substr(1);

	if (target.empty() || message.empty())
	{
		_sendMessage(clientFd, ":ircserv 411 " + sender->getNickname() + " :No recipient given (PRIVMSG)\r\n");
		return;
	}

	if (target[0] == '#')//si es un canal
	{
		if (_channels.find(target) == _channels.end())
			return _sendMessage(clientFd, ":ircserv 403 " + sender->getNickname() + " " + target + " :No such channel\r\n");

		Channel *channel = _channels[target];
		if (!channel->hasClient(sender))
			return _sendMessage(clientFd, ":ircserv 442 " + sender->getNickname() + " " + target + " :You're not on that channel\r\n");

		std::string fullMsg = prefix + target + " :" + message + "\r\n";//arregla el mensaje
		for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second != sender && channel->hasClient(it->second))//si no es el sender
				_sendMessage(it->first, fullMsg);
		}
	}
	else
	{
		Client *recipient = NULL;//encontra el destinatario del mensagem
		for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second->getNickname() == target)
			{
				recipient = it->second;
				break;
			}
		}

		if (!recipient)
			return _sendMessage(clientFd, ":ircserv 401 " + sender->getNickname() + " " + target + " :No such nick\r\n");

		std::string fullMsg = prefix + target + " :" + message + "\r\n";//arregla el mensaje
		_sendMessage(recipient->getFd(), fullMsg);//envia
	}
}

void Server::_ping(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, token;

	ss >> cmd >> token;
	if (!token.empty() && token[0] == ':')
		token = token.substr(1);
	if (!token.empty())
		_sendMessage(clientFd, ":" + client->getNickname() + " PONG :" + token + "\r\n");
}

void Server::_part(Client *client, int clientFd, const std::string &msg)
{
	std::string cmd, channelName, reason;
	std::istringstream ss(msg);

	ss >> cmd >> channelName >> reason;
	std::getline(ss, reason);
	if (reason[0] == ':')
		reason = reason.substr(1);
	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ":ircserv 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n");
	
	Channel *channel = _channels[channelName];
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n");
	
	std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName;
	if (!reason.empty())
	partMsg += " :" + reason;
	partMsg += "\r\n";
	
	_broadcastToChannel(channelName, partMsg, -1);//envia mensaje al canal
	
	channel->removeClient(client);//remove el cliente
	
	bool empty = true;//si el canal estiver vacio al final remove el canal tambien
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (channel->hasClient(it->second))
		{
			empty = false;
			break;
		}
	}
	if (empty)
	{
		delete channel;
		_channels.erase(channelName);
	}
}

/* void Server::_handleNick(int clientFd, const std::string &nickname)
{
	Client *client = _clients[clientFd];
	client->setNickname(nickname);

	if (client->isRegistered())
		_sendWelcomeMessage(clientFd);
} */

/* void Server::_handleUser(int clientFd, const std::string &username)
{
	Client *client = _clients[clientFd];
	client->setUsername(username);

	if (client->isRegistered())
		_sendWelcomeMessage(clientFd);
} */

/* void Server::_handleQuit(int clientFd, const std::string &reason)
{
	Client *client = _clients[clientFd];
	std::string quitMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT";
	if (!reason.empty())
		quitMsg += " :" + reason;
	else
		quitMsg += " :Client Quit";
	quitMsg += "\r\n";

	for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->second->hasClient(client))
		{
			_broadcastToChannel(it->first, quitMsg, clientFd);
			it->second->removeClient(client);
		}
	}

	for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		bool empty = true;
		for (std::map<int, Client *>::iterator cit = _clients.begin(); cit != _clients.end(); ++cit)
		{
			if (it->second->hasClient(cit->second))
			{
				empty = false;
				break;
			}
		}

		if (empty)
		{
			delete it->second;
			_channels.erase(it++);
		}
		else
			++it;
	}
	
	_sendMessage(clientFd, quitMsg);
	close(clientFd);
	delete client;
	_clients.erase(clientFd);
}
 */
