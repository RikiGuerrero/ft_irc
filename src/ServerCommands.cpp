#include "Server.hpp"
#include "IrcMessages.hpp"

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
				_sendMessage(clientFd, ERR_PASSWDMISMATCH);
		}
		else if (cmd == "NICK")
		{
			if (!client->isPassSet())
			{
				_sendMessage(clientFd, ERR_NOTREGISTERED);
				return;
			}
			std::string nick;
			ss >> nick;
			if (_nicknameExists(nick))
				_sendMessage(clientFd, ERR_NICKNAMEINUSE(nick));
			else
				client->setNickname(nick);
		}
		else if (cmd == "USER")
		{
			if (!client->isPassSet())
			{
				_sendMessage(clientFd, ERR_NOTREGISTERED);
				return;
			}
			if (client->isRegistered())
			{
				_sendMessage(clientFd, ERR_ALREADYREGISTRED);
				return;
			}
			std::string username, unused, unused2, realname;
			ss >> username >> unused >> unused2;
			std::getline(ss, realname);
			if (username.empty() || unused.empty() || unused2.empty() || realname.empty())
			{
				_sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "USER"));
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
				_sendMessage(clientFd, RPL_PONG(token));
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
		else if (cmd == "QUIT")
		{
			std::string reason;
			std::getline(ss, reason);
			if (reason[0] == ':')
				reason = reason.substr(1);
			_handleQuit(clientFd, reason);
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
	
	_sendMessage(clientFd, RPL_JOIN(client->getNickname(), client->getUsername(), client->getHostname(), channelName));
	
	std::string names = RPL_NAMREPLY(channelName);
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

	_sendMessage(clientFd, RPL_ENDOFNAMES(client->getNickname(), channelName));

	_broadcastToChannel(channelName, client->getPrefix() + " JOIN :" + channelName + "\r\n", clientFd);
}

void Server::_handlePrivmsg(int clientFd, const std::string &target, const std::string &message)
{
	Client *sender = _clients[clientFd];
	std::string prefix = ":" + sender->getNickname() + "!" + sender->getUsername() + "@localhost ";

	if (target.empty() || message.empty())
	{
		_sendMessage(clientFd, ERR_NEEDMOREPARAMS(sender->getNickname(), "PRIVMSG"));
		return;
	}

	if (target[0] == '#')
	{
		if (_channels.find(target) == _channels.end())
		{
			_sendMessage(clientFd, ERR_NOSUCHCHANNEL(sender->getNickname(), target));
			return;
		}

		Channel *channel = _channels[target];
		if (!channel->hasClient(sender))
		{
			_sendMessage(clientFd, ERR_NOTONCHANNEL(sender->getNickname(), target));
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
			_sendMessage(clientFd, ERR_NOSUCHNICK(sender->getNickname(), target));
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
		_sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
		return;
	}

	Channel *channel = _channels[channelName];
	if (!channel->hasClient(client))
	{
		_sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channelName));
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
	_removeClient(clientFd);
}

void Server::_sendWelcomeMessage(int clientFd)
{
	Client *client = _clients[clientFd];
	const std::string &nick = client->getNickname();
	const std::string &username = client->getUsername();
	const std::string &hostname = client->getHostname();

	_sendMessage(clientFd, RPL_WELCOME(nick, username, hostname));
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