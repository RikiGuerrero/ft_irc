#include "Server.hpp"
#include "IrcMessages.hpp"

void Server::_pass(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string pass, cmd; 
	ss >> cmd >> pass;
	if (pass == _password)
		client->setPass(true);
	else
		_sendMessage(clientFd, ERR_PASSWDMISMATCH);
}

void Server::_nick(Client *client, int clientFd, const std::string &msg)
{
	std::string nick, cmd;
	std::istringstream ss(msg);
	ss >> cmd;
	if (!client->isPassSet())
		return _sendMessage(clientFd, ERR_NOTREGISTERED);
	ss >> nick;
	if (_nicknameExists(nick))
		_sendMessage(clientFd, ERR_NICKNAMEINUSE(nick));
	else
		client->setNickname(nick);
}

void Server::_user(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string username, unused, unused2, realname, cmd;
	if (!client->isPassSet())
		return _sendMessage(clientFd, ERR_NOTREGISTERED);
	if (client->isRegistered())
		return _sendMessage(clientFd, ERR_ALREADYREGISTRED);
	ss >> cmd >> username >> unused >> unused2;
	std::getline(ss, realname);
	if (username.empty() || unused.empty() || unused2.empty() || realname.empty())
		return _sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "USER"));	
	if (!realname.empty() && realname[0] == ' ')
		realname = realname.substr(1);
	if (!realname.empty() && realname[0] == ':')
		realname = realname.substr(1);
	client->setUsername(username);
	client->setRealname(realname);
}

void Server::	_join(Client *client, int clientFd, const std::string &msg)
{
	std::string cmd, channels, keys;
	std::istringstream ss(msg);
	ss >> cmd >> channels >> keys;

	std::vector<std::string> channelList;
	std::vector<std::string> keyList;
	std::istringstream channelStream(channels);
	std::istringstream keyStream(keys);
	std::string token;

	while (std::getline(channelStream, token, ','))
		channelList.push_back(token);
	while (std::getline(keyStream, token, ','))
		keyList.push_back(token);
	
	for (size_t i = 0; i < channelList.size(); ++i)
	{
		std::string channelName = channelList[i];
		std::string pass = (i < keyList.size()) ? keyList[i] : "";
		
		if (channelName.empty() || channelName[0] != '#')
		{
			_sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
			continue;
		}
		if (_channels.find(channelName) == _channels.end())
			_channels[channelName] = new Channel(channelName);
		Channel *channel = _channels[channelName];
		if (channel->hasClient(client))
			continue;

		if (channel->getKeyNeed())
		{
			if (pass != channel->getPass() || pass.empty())
			{
				_sendMessage(clientFd, ERR_BADCHANNELKEY(client->getNickname(), channelName));
				continue;
			}
		}
		if (channel->getLimit() != -1)
		{
			if (channel->getTotalUsers() == channel->getLimit())
			{
				_sendMessage(clientFd, ERR_CHANNELISFULL(client->getNickname(), channelName));
				continue;
			}
		}
		if (channel->getInviteOlny())
		{
			if (!channel->isInvited(clientFd))
			{
				_sendMessage(clientFd, ERR_INVITEONLYCHAN(client->getNickname(), channelName));
				continue;
			}
		}
		_acceptClient(channel, client, clientFd, channelName);
	}
}


void Server::_acceptClient(Channel *channel, Client *client, int clientFd, const std::string &channelName)
{
	channel->addClient(client);
	if (!channel->isOperator(client) && channel->getTotalUsers() == 1)
		channel->addOperator(client);
	_sendMessage(clientFd, RPL_JOIN(client->getNickname(), client->getUsername(), client->getHostname(), channelName));
	if (channel->getTopic().empty())
		_sendMessage(clientFd, RPL_NOTOPIC(client->getNickname(), channelName));
	else
		_sendMessage(clientFd, RPL_TOPIC(client->getNickname(), channelName, channel->getTopic()));
	std::string names = RPL_NAMREPLY(client->getNickname(), channelName);
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

void Server::_privmsg(Client *sender, int clientFd, const std::string &msg)
{
	std::string cmd, target, message;
	std::istringstream ss(msg);
	std::string prefix = ":" + sender->getNickname() + "!" + sender->getUsername() + "@localhost PRIVMSG ";

	ss >> cmd >> target;
	std::getline(ss, message);
	if (!message.empty() && message[0] == ' ')
		message = message.substr(1);
	if (!message.empty() && message[0] == ':')
		message = message.substr(1);

	if (target.empty() || message.empty())
	{
		_sendMessage(clientFd, ERR_NORECIPIENT(sender->getNickname(), "PRIVMSG"));
		return;
	}

	if (target[0] == '#')
	{
		if (_channels.find(target) == _channels.end())
			return _sendMessage(clientFd, ERR_NOSUCHCHANNEL(sender->getNickname(), target));

		Channel *channel = _channels[target];
		if (!channel->hasClient(sender))
			return _sendMessage(clientFd, ERR_NOTONCHANNEL(sender->getNickname(), target));

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
			return _sendMessage(clientFd, ERR_NOSUCHNICK(sender->getNickname(), target));
		std::string fullMsg = prefix + target + ": " + message + "\r\n";
		_sendMessage(recipient->getFd(), fullMsg);
	}
}

void Server::_ping(int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, token;

	ss >> cmd >> token;
	if (!token.empty() && token[0] == ':')
		token = token.substr(1);
	if (!token.empty())
		_sendMessage(clientFd, RPL_PONG(token));
}

void Server::_part(Client *client, int clientFd, const std::string &msg)
{
	std::string cmd, channels, reason;
	std::istringstream ss(msg);

	ss >> cmd >> channels;
	std::getline(ss, reason);
	if (!reason.empty() && reason[0] == ' ')
		reason = reason.substr(1);
	if (!reason.empty() && reason[0] == ':')
		reason = reason.substr(1);

	std::vector<std::string> channelList;
	std::string token;
	std::istringstream channelStream(channels);
	while (std::getline(channelStream, token, ','))
		channelList.push_back(token);
	
	for (size_t i = 0; i < channelList.size(); ++i)
	{
		std::string channelName = channelList[i];
		if (_channels.find(channelName) == _channels.end())
		{
			_sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
			continue;
		}
		Channel *channel = _channels[channelName];
		if (!channel->hasClient(client))
		{
			_sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channelName));
			continue;
		}

		std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName;
		if (!reason.empty())
			partMsg += " :" + reason;
		partMsg += "\r\n";
		_broadcastToChannel(channelName, partMsg, -1);
		
		channel->removeClient(client);
		
		if (channel->getTotalUsers() > 0)
		{
			bool hasOperator = false;
			for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
			{
				if (channel->hasClient(it->second) && channel->isOperator(it->second))
				{
					hasOperator = true;
					break;
				}
			}
			if (!hasOperator)
			{
				for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
				{
					if (channel->hasClient(it->second))
					{
						_broadcastToChannel(channel->getName(), ":ft_irc MODE " + channel->getName() + " +o " + it->second->getNickname() + "\r\n");					
						channel->addOperator(it->second);
						break;
					}
				}
			}
		}
		if (channel->getTotalUsers() == 0)
		{
			delete channel;
			_channels.erase(channelName);
		}
	}
}

void Server::_quit(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, reason;

	ss >> cmd;
	std::getline(ss, reason);
	if (!reason.empty() && reason[0] == ':')
		reason = reason.substr(1);
	std::string quitMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT";
	if (!reason.empty())
		quitMsg += " :" + reason;
	else
		quitMsg += " :Client Quit";
	quitMsg += "\r\n";

	std::vector<std::string> channelsToLeave;
	for (std::map<std::string, Channel *>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->second->hasClient(client))
			channelsToLeave.push_back(it->first);
	}	
	for (std::vector<std::string>::iterator it = channelsToLeave.begin(); it != channelsToLeave.end(); ++it)
		_broadcastToChannel(*it, quitMsg, clientFd);
	for (std::vector<std::string>::iterator it = channelsToLeave.begin(); it != channelsToLeave.end(); ++it)
		_part(client, clientFd, "PART " + *it + " :" + reason);

	_sendMessage(clientFd, quitMsg);
	_removeClient(clientFd);
}