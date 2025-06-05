#include "Server.hpp"
#include "IrcMessages.hpp"


void Server::_modeO(Client *client, int clientFd, std::string &flag, std::string &user, Channel *channel)
{
	if (client->getNickname() == user)
		return;
	Client *target = NULL;
	if (user.empty())
		return _sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "MODE +o"));	
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ERR_CHANOPRIVSNEEDED(client->getNickname(), channel->getName()));
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channel->getName()));
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == user)
		{
			target = it->second;
			break;
		}
	}
	if (!target)
		return _sendMessage(clientFd, ERR_USERNOTINCHANNEL(client->getNickname(), user, channel->getName()));
	if (flag[0] == '+')
		channel->addOperator(target);
	else if (flag[0] == '-')
		channel->removeOperator(target);
	_broadcastToChannel(channel->getName(), ":ft_irc MODE " + channel->getName() + " " + flag + " " + user + "\r\n");
}

void Server::_modeI(Channel *channel, const std::string &flag)
{
	if (flag[0] == '-')
		channel->setInviteOnly(false);
	if (flag[0] == '+')
		channel->setInviteOnly(true);
	_broadcastToChannel(channel->getName(), ":ft_irc MODE " + channel->getName() + " " + flag + " \r\n");
}

void Server::_modeT(Channel *channel, const std::string &flag)
{
	if (flag[0] == '-')
		channel->setTopicOpMode(false);
	if (flag[0] == '+')
		channel->setTopicOpMode(true);
	_broadcastToChannel(channel->getName(), ":ft_irc MODE " + channel->getName() + " " + flag + " \r\n");
}

void Server::_modeL(Client *client, Channel *channel, const std::string &flag, const std::string &parameters)
{

	for (size_t i = 0; i < parameters.size(); i++)
	{
		if (!std::isdigit(parameters[i]))
			return ;
	}
	if (std::atoi(parameters.c_str()) < 1)
		return ;
	if (flag[0] == '+')
	{
		if (parameters.empty())
			return _sendMessage(client->getFd(), ERR_NEEDMOREPARAMS(client->getNickname(), " MODE +l"));
		channel->setLimit(atoi(parameters.c_str()));
	}
	else if (flag[0] == '-')
		channel->setLimit(-1);
	_broadcastToChannel(channel->getName(), ":ft_irc MODE " + channel->getName() + " " + flag + " " +  parameters + " \r\n");
}

void Server::_modeK(Client *client, Channel *channel, const std::string &flag, const std::string &parameters)
{
	if (flag[0] == '+')
	{
		if (parameters.empty())
			return _sendMessage(client->getFd(), ERR_NEEDMOREPARAMS(client->getNickname(), " MODE +k"));
		channel->setPass(parameters, true);
	}
	else if (flag[0] == '-')
		channel->setPass("", false);
	_broadcastToChannel(channel->getName(), ":ft_irc MODE " + channel->getName() + " " + flag + " \r\n");
}

void Server::_mode(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, channelName, flag, parameters;

	ss >> cmd >> channelName >> flag >> parameters;
	if (_channels.find(channelName) == _channels.end())
	return _sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
	Channel *channel = _channels[channelName];
	if (!channel->isOperator(client))
	return _sendMessage(clientFd, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
	if (flag.empty())
	return _sendMessage(clientFd, RPL_CHANNELMODES(client->getNickname(), channelName, channel->getModes()));
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