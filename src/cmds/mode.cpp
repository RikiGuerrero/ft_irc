#include "Server.hpp"


void Server::_modeO(Client *client, int clientFd, std::string &flag, std::string &user, Channel *channel)
{
	Client *target = NULL;//encontra el target del mensaje
	if (user.empty())
		return _sendMessage(clientFd, ":ircserv 461 " + client->getNickname() + " MODE :Not enough parameters\r\n");	
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ":ircserv 482 " + client->getNickname() + " " + channel->getName() + " :You're not channel operator\r\n");
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channel->getName() + " :You're not on that channel\r\n");
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == user)
		{
			target = it->second;
			break;
		}
	}
	if (channel->hasClient(target))
		return _sendMessage(clientFd, ":ircserv 441 " + client->getNickname() + " " + target->getNickname() + " " + channel->getName() + " :They aren't on that channel\r\n");
	if (flag[0] == '+')
		channel->addOperator(target);
	else if (flag[0] == '-')
		channel->removeOperator(target);
}

void Server::_modeI(Channel *channel, const std::string &flag)
{
	if (flag[0] == '-')
		channel->setInviteOnly(false);
	if (flag[0] == '+')
		channel->setInviteOnly(true);
}

void Server::_modeT(Channel *channel, const std::string &flag)
{
	if (flag[0] == '-')
		channel->setTopicOpMode(false);
	if (flag[0] == '+')
		channel->setTopicOpMode(true);
}

void Server::_modeL(Client *client, Channel *channel, const std::string &flag, const std::string &parameters)
{
	if (flag[0] == '+')
	{
		if (parameters.empty())
			return _sendMessage(client->getFd(), ":ircserv 461 " + client->getNickname() + " MODE +l :Not enough parameters\r\n");
		channel->setLimit(atoi(parameters.c_str()));
	}
	else if (flag[0] == '-')
		channel->setLimit(-1);
}

void Server::_modeK(Client *client, Channel *channel, const std::string &flag, const std::string &parameters)
{
	if (flag[0] == '+')
	{
		if (parameters.empty())
			return _sendMessage(client->getFd(), ":ircserv 461 " + client->getNickname() + " MODE +k :Not enough parameters\r\n");
		channel->setPass(parameters, true);
	}
	else if (flag[0] == '-')
		channel->setLimit(-1);
}

void Server::_mode(Client *client, int clientFd, const std::string &msg)
{//estoy considerando que las flags no vienen como ++oi solo +i +o
	std::istringstream ss(msg);
	std::string cmd, channelName, flag, parameters;

	ss >> cmd >> channelName >> flag >> parameters;

	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ":ircserv 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n");
	Channel *channel = _channels[channelName];
	if (!channel->isOperator(client))//esos comandos son especificos del operador
		return _sendMessage(clientFd, ":ircserv 482 " + client->getNickname() + " " + channelName + " You're not channel operator\r\n");
	if (flag.empty())
		return _sendMessage(clientFd, ":ircserv 324 "+ client->getNickname() + " " + channelName  + channel->getModes() + "\r\n");
	if (flag[1] == 'o')
		return _modeO(client, clientFd, flag, parameters, channel);
	if (flag[1] == 'i')
		return _modeI(channel, flag);
	if (flag[1] == 't')
		return _modeT(channel, flag);
	if (flag[1] == 'l')
		return _modeL(client, channel, flag, parameters);
	if (flag[1] == 'k')
		return _modeK(client, channel, flag, parameters);
	
}