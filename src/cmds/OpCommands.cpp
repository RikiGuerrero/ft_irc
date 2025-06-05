#include "Server.hpp"
#include "IrcMessages.hpp"

void Server::_topic(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, channelName, newTopic;

	ss >> cmd >> channelName;
	std::getline(ss, newTopic);
	if (channelName.empty())
		return _sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "TOPIC"));	
	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
	Channel *channel = _channels[channelName];
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channelName));	
	if (newTopic.empty())
	{
		if (channel->getTopic().empty())
			return _sendMessage(clientFd, RPL_NOTOPIC(client->getNickname(), channelName));
		else 
			return _sendMessage(clientFd, RPL_TOPIC(client->getNickname(), channelName, channel->getTopic()));
	}
	else if (channel->getTopicOpMode() == true && !channel->isOperator(client))
		return _sendMessage(clientFd, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
	else
	{
		if (newTopic == ":")
		{
			channel->setTopic("");
			_broadcastToChannel(channelName,  "Clearing the topic on " + channelName + "\r\n", -1);
		}
		
		else
		{
			channel->setTopic(newTopic);
			_broadcastToChannel(channelName,  "Setting the topic on " + channelName + " to " + newTopic + "\r\n", -1);
		}
	}

}

void Server::_invite(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, user, channelName;

	ss >> cmd >> user >> channelName;
	
	if (user.empty() || channelName.empty())
		return _sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "INVITE"));	
	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(),channelName));
	Channel *channel = _channels[channelName];
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channelName));
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
	Client *target = NULL;
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->second->getNickname() == user)
		{
			target = it->second;
			break;
		}
	}
	if (!target)
		return _sendMessage(clientFd, ERR_NOSUCHNICK(user, channelName));
	if (channel->hasClient(target))
		return _sendMessage(target->getFd(), ERR_USERONCHANNEL(client->getNickname(), user, channelName));
	channel->addInvited(target);
	_sendMessage(clientFd, RPL_INVITING(msg));
	_sendMessage(target->getFd(), client->getNickname() + " has invited you to the channel " + channelName + "\r\n");
}

void Server::_kick(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, channelName, reason, rest, token, users_str;
	std::vector<std::string> users_vec;
	
	ss >> cmd >> channelName >> rest >> users_str;
	std::getline(ss, reason);
	if (rest[0] != '#')
	{
		reason = users_str + reason;
		users_str = rest;
	}
	std::istringstream sss(users_str);
	while (std::getline(sss, token, ','))
	{
		if (!token.empty() && token[0] == ' ')
			token = token.substr(1);
		if (!token.empty() && token[0] == ':')
			token = token.substr(1);
		users_vec.push_back(token);
	}

	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
	Channel *channel = _channels[channelName];
	if (channelName.empty() || users_vec.empty())
		return _sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "TOPIC"));
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channelName));

	for (std::vector<std::string>::iterator vit = users_vec.begin(); vit != users_vec.end(); vit++)
	{	
		Client *target = NULL;
		for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second->getNickname() == *vit)
			{
				target = it->second;
				break;
			}
		}
		if (!target || !channel->hasClient(target))
			_sendMessage(clientFd, ERR_USERNOTINCHANNEL(client->getNickname(), *vit, channelName));
		else if (client->getNickname() == *vit)
			_sendMessage(clientFd, ERR_NOSUCHNICK(client->getNickname(), *vit));
		else
		{
			std::string kickMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost KICK ";
			if (reason.empty())
				reason = "being too boring";
			_broadcastToChannel(channelName, kickMsg + channelName + " " + target->getNickname() + " " + reason + "\r\n");
			_sendMessage(target->getFd(), target->getNickname() + " was kicked from " + channelName + " due to " + reason + "\r\n");
			channel->removeClient(target);
		}
	}
}