#include "Server.hpp"

//posso passar ss como ponteiro.
void Server::_pass(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string pass, cmd; 
	ss >> cmd >> pass;
	if (pass == _password)
		client->setPass(true);
	else
		_sendMessage(clientFd, "464 :Password incorrect\r\n");//envia mensagem ao servidor
}

void Server::_nick(Client *client, int clientFd, const std::string &msg)
{
	std::string nick, cmd;
	std::istringstream ss(msg);
	ss >> cmd;
	if (!client->isPassSet())
	{
		_sendMessage(clientFd, "451 :You have not registered\r\n");
		return;
	}
	ss >> nick;
	if (_nicknameExists(nick))//se nao é repetido
		_sendMessage(clientFd, "433 * " + nick + " :Nickname is already in use\r\n");
	else
		client->setNickname(nick);
}

void Server::_user(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string username, unused, unused2, realname, cmd;

	if (!client->isPassSet())
	{
		_sendMessage(clientFd, "451 :You have not registered\r\n");
		return;
	}
	if (client->isRegistered())
	{
		_sendMessage(clientFd, "462 :You may not reregister\r\n");
		return;
	}
	ss >> cmd >> username >> unused >> unused2;
	std::getline(ss, realname);
	if (username.empty() || unused.empty() || unused2.empty() || realname.empty())
	{
		_sendMessage(clientFd, "461 USER :Not enough parameters\r\n");
		return;
	
	if (!realname.empty() && realname[0] == ' ')
		realname = realname.substr(1);
	if (!realname.empty() && realname[0] == ':')
		realname = realname.substr(1);
	client->setUsername(username);
	client->setRealname(realname);
	}
}

void Server::_join(Client *client, int clientFd, const std::string &msg)
{
	std::string cmd, channelName;
	std::istringstream ss(msg);
	ss >> cmd >> channelName;

	if (_channels.find(channelName) == _channels.end())// si el canal no existe
		_channels[channelName] = new Channel(channelName);//cria o canal
	
	Channel *channel = _channels[channelName];

	if (channel->hasClient(client))//se o cliente esta no canal
		return;
	
	channel->addClient(client);//se nao adiciona ao set dos clientes

	if (channel->isOperator(client) == false && channel->getTopic().empty())//se o cliente nao é operador e o tópico esta vazio
		channel->addOperator(client);//adc ao set de operadores
	
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

	if (target[0] == '#')//se é um canal
	{
		if (_channels.find(target) == _channels.end())
		{
			_sendMessage(clientFd, ":ircserv 403 " + sender->getNickname() + " " + target + " :No such channel\r\n");
			return;
		}

		Channel *channel = _channels[target];
		if (!channel->hasClient(sender))
		{
			_sendMessage(clientFd, ":ircserv 442 " + sender->getNickname() + " " + target + " :You're not on that channel\r\n");
			return;
		}

		std::string fullMsg = prefix + target + " :" + message + "\r\n";//ajeita a mensagem
		for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second != sender && channel->hasClient(it->second))//se nao for o sender
				_sendMessage(it->first, fullMsg);
		}
	}
	else
	{
		Client *recipient = NULL;//encontra o destinatario da mensagem
		for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second->getNickname() == target)
			{
				recipient = it->second;
				break;
			}
		}

		if (!recipient)
		{
			_sendMessage(clientFd, ":ircserv 401 " + sender->getNickname() + " " + target + " :No such nick\r\n");
			return;
		}

		std::string fullMsg = prefix + target + " :" + message + "\r\n";//ajeita a mensagem
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
	{
		_sendMessage(clientFd, ":ircserv 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n");
		return;
	}
	
	Channel *channel = _channels[channelName];
	if (!channel->hasClient(client))
	{
		_sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n");
		return;
	}
	
	std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName;
	if (!reason.empty())
	partMsg += " :" + reason;
	partMsg += "\r\n";
	
	_broadcastToChannel(channelName, partMsg, -1);//envia mensagem ao canal
	
	channel->removeClient(client);//remove o cliente
	
	bool empty = true;//si o canal estiver vazio após isso remove o canal também
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

void Server::_parseCommand(int clientFd, const std::string &msg)
{
	Client *client = _clients[clientFd];
	std::istringstream ss(msg);
	std::string cmd;
	ss >> cmd;

	bool wasAuthenticated = client->isAuthenticated();

	if (!wasAuthenticated)//se nao esta autenticado
	{
		if (cmd == "PASS" || cmd == "pass")
			_pass(client, clientFd, msg);
		else if (cmd == "NICK" || cmd == "nick")
			_nick(client, clientFd, msg);
		else if (cmd == "USER")
			_user(client, clientFd, msg);
	}
	else
	{
		if (cmd == "JOIN")
			_join(client, clientFd, msg);//entrar em um canal
		else if (cmd == "PRIVMSG")
			_privmsg(client, clientFd, msg);//enviar msg privada a um canal ou cliente
		else if (cmd == "PING")
			_ping(client, clientFd, msg);
		else if (cmd == "PART")//remove um usuario de um canal
			_part(client, clientFd, msg);
	}

	client->tryAuthenticate();//autentica o cliente

	if (!wasAuthenticated && client->isAuthenticated())
		_sendWelcomeMessage(clientFd);
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
void Server::_sendWelcomeMessage(int clientFd)
{
	Client *client = _clients[clientFd];
	const std::string &nick = client->getNickname();
	const std::string &user = client->getUsername();

	_sendMessage(clientFd, ":ircserv 001 " + nick + " :Welcome to the IRC server " + nick + "!" + user + "@localhost\r\n");
	_sendMessage(clientFd, ":ircserv 002 " + nick + " :Your host is ircserv, running version 1.0\r\n");
	_sendMessage(clientFd, ":ircserv 003 " + nick + " :This server was created just now\r\n");
	_sendMessage(clientFd, ":ircserv 376 " + nick + " :End of MOTD command\r\n");
}

void Server::_broadcastToChannel(const std::string &channelName, const std::string &msg, int excludeFd)
{
	Channel* channel = _channels[channelName];//encontra o canal desejado e envia a mensagem
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (channel->hasClient(it->second) && it->first != excludeFd)
			_sendMessage(it->first, msg);
	}
}
