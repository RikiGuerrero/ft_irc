#include "Server.hpp"

void Server::_topic(Client *client, int clientFd, const std::string &msg)//NO TESTEADO
{
	std::istringstream ss(msg);
	std::string cmd, channelName, newTopic;

	if (channelName.empty())
		return _sendMessage(clientFd, ":ircserv 461 " + client->getNickname() + " TOPIC :Not enough parameters\r\n");	
	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ":ircserv 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n");
	Channel *channel = _channels[channelName];
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channelName + " :You're not on that channel");	
	if (newTopic.empty()) //solo mirar el topico
	{
		if (channel->getTopic().empty())
			return _sendMessage(clientFd, ":ircserver 331 " + client->getNickname() + " " + channelName + " :No topic is set");
		else 
			return _sendMessage(clientFd, ":ircserver 332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic());
	}
	else if (channel->getTopicOpMode() == true && !channel->isOperator(client))//si solo puede cambiar los operadores y el cliente no lo es
		return _sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channelName + " You're not on that channel");
	else
	{
		if (newTopic == ":")
		{
			channel->setTopic("");
			_broadcastToChannel(channelName,  "Clearing the topic on " + channelName, -1);
		}
		
		else
		{
			channel->setTopic(newTopic);
			_broadcastToChannel(channelName,  "Setting the topic on " + channelName + " to " + newTopic, -1);
		}
	}

}

void Server::_invite(Client *client, int clientFd, const std::string &msg)//NO TESTEADO
{
	std::istringstream ss(msg);
	std::string cmd, user, channelName;

	ss >> cmd >> user >> channelName;
	if (user.empty() || channelName.empty())
		return _sendMessage(clientFd, ":ircserv 461 " + client->getNickname() + " INVITE :Not enough parameters\r\n");	
	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ":ircserv 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n");
	Channel *channel = _channels[channelName];
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channelName + " You're not on that channel");
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ":ircserv 482 " + client->getNickname() + " " + channelName + " You're not channel operator");
	Client *target = NULL;
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		if (it->second->getNickname() == user)
			target = it->second;
		break;
	}
	if (target)
		return _sendMessage(target->getFd(), ":ircserv 443 " + client->getNickname() + " " + user + " " + channelName + " is already on channel");
	channel->addInvited(target);
	_sendMessage(clientFd, msg);//mensage a quien invito 
	_sendMessage(target->getFd(), client->getNickname() + " has invited you to the channel " + channelName);
}

void Server::_kick(Client *client, int clientFd, const std::string &msg)//NO TESTEADO
{	
	std::istringstream ss(msg);
	std::string cmd, channelName, user, reason;
	ss >> cmd >> channelName >> user >> reason;

	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ":ircserv 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n");
	Channel *channel = _channels[channelName];
	if (channelName.empty() || user.empty())
		return _sendMessage(clientFd, ":ircserv 461 " + client->getNickname() + " KICK :Not enough parameters\r\n");
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ":ircserv 482 " + client->getNickname() + " " + channelName + " :You're not channel operator");
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channelName + " :You're not on that channel");
	
	Client *target = NULL;//encontra el target del mensaje
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == user)
		{
			target = it->second;
			break;
		}
	}
	if (!target)
		return _sendMessage(clientFd, ":ircserv 441 " + client->getNickname() + " " + target->getNickname() + channelName + " :They aren't on that channel");
	if (reason.empty())
		reason = "being too boring";
	_broadcastToChannel(channelName, target->getNickname() + " was kicked from " + channelName + " due to " + reason);
	_removeClient(target->getFd());
}