#include "Server.hpp"

void Server::_parseCommand(int clientFd, const std::string &msg)
{
	Client *client = _clients[clientFd];
	std::istringstream ss(msg);
	std::string cmd;
	ss >> cmd;

	bool wasAuthenticated = client->isAuthenticated();

	if (!wasAuthenticated)
	{
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
			if (!client->isPassSet())
			{
				_sendMessage(clientFd, "451 :You have not registered\r\n");
				return;
			}
			std::string nick;
			ss >> nick;
			if (_nicknameExists(nick))
				_sendMessage(clientFd, "433 * " + nick + " :Nickname is already in use\r\n");
			else
				client->setNickname(nick);
		}
		else if (cmd == "USER")
		{
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
			std::string username, unused, unused2, realname;
			ss >> username >> unused >> unused2;
			std::getline(ss, realname);
			if (username.empty() || unused.empty() || unused2.empty() || realname.empty())
			{
				_sendMessage(clientFd, "461 USER :Not enough parameters\r\n");
				return;
			}

			if (!realname.empty() && realname[0] == ' ')
				realname = realname.substr(1);
			if (!realname.empty() && realname[0] == ':')
				realname = realname.substr(1);
			client->setUsername(username);
			client->setRealname(realname);
		}
	}
	else
	{
		if (cmd == "JOIN")
		{
			std::string channel;
			ss >> channel;
			_handleJoin(clientFd, channel);
		}
		else if (cmd == "PRIVMSG")
		{
			std::string target, message;
			ss >> target;
			std::getline(ss, message);
			if (message[0] == ':')
				message = message.substr(1);
			_handlePrivmsg(clientFd, target, message);
		}
		else if (cmd == "PING")
		{
			std::string token;
			ss >> token;
			if (!token.empty() && token[0] == ':')
				token = token.substr(1);
			if (!token.empty())
				_sendMessage(clientFd, ":" + client->getNickname() + " PONG :" + token + "\r\n");
		}
		else if (cmd == "PART")
		{
			std::string channel, reason;
			ss >> channel;
			std::getline(ss, reason);
			if (reason[0] == ':')
				reason = reason.substr(1);
			_handlePart(clientFd, channel, reason);
		}
	}

	client->tryAuthenticate();

	if (!wasAuthenticated && client->isAuthenticated())
		_sendWelcomeMessage(clientFd);
}

void Server::_handleNick(int clientFd, const std::string &nickname)
{
	Client *client = _clients[clientFd];
	client->setNickname(nickname);

	if (client->isRegistered())
		_sendWelcomeMessage(clientFd);
}

void Server::_handleUser(int clientFd, const std::string &username)
{
	Client *client = _clients[clientFd];
	client->setUsername(username);

	if (client->isRegistered())
		_sendWelcomeMessage(clientFd);
}

void Server::_handleJoin(int clientFd, const std::string &channelName)
{
	Client* client = _clients[clientFd];

	if (_channels.find(channelName) == _channels.end())
		_channels[channelName] = new Channel(channelName);
	
	Channel *channel = _channels[channelName];

	if (channel->hasClient(client))
		return;
	
	channel->addClient(client);

	if (channel->isOperator(client) == false && channel->getTopic().empty())
		channel->addOperator(client);
	
	_sendMessage(clientFd, ":" + client->getNickname() + " JOIN :" + channelName + "\r\n");

	if (channel->getTopic().empty())
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

	_broadcastToChannel(channelName, client->getPrefix() + " JOIN :" + channelName + "\r\n", clientFd);
}

void Server::_handlePrivmsg(int clientFd, const std::string &target, const std::string &message)
{
	Client *sender = _clients[clientFd];
	std::string prefix = ":" + sender->getNickname() + "!" + sender->getUsername() + "@localhost PRIVMSG ";

	if (target.empty() || message.empty())
	{
		_sendMessage(clientFd, ":ircserv 411 " + sender->getNickname() + " :No recipient given (PRIVMSG)\r\n");
		return;
	}

	if (target[0] == '#')
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

		std::string fullMsg = prefix + target + " :" + message + "\r\n";
		for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second != sender && channel->hasClient(it->second))
				_sendMessage(it->first, fullMsg);
		}
	}
	else
	{
		Client *recipient = NULL;
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

		std::string fullMsg = prefix + target + " :" + message + "\r\n";
		_sendMessage(recipient->getFd(), fullMsg);
	}
}

void Server::_handlePart(int clientFd, const std::string &channelName, const std::string &reason)
{
	Client *client = _clients[clientFd];

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

	_broadcastToChannel(channelName, partMsg, -1);

	channel->removeClient(client);

	bool empty = true;
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

void Server::_handleQuit(int clientFd, const std::string &reason)
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
	Channel* channel = _channels[channelName];
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (channel->hasClient(it->second) && it->first != excludeFd)
			_sendMessage(it->first, msg);
	}
}