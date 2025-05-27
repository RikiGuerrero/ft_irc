#include "Server.hpp"


void Server::_modeO(Client *client, int clientFd, std::string &flag, std::string &user, Channel *channel)
{
	Client *target = NULL;//encontra el target del mensaje
	if (user.empty())
		return _sendMessage(clientFd, ":ircserv 461 " + client->getNickname() + " MODE :Not enough parameters\r\n");	
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ":ircserv 482 " + client->getNickname() + " " + channel->getName() + " :You're not channel operator");
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channel->getName() + " :You're not on that channel");
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == user)
		{
			target = it->second;
			break;
		}
	}
	if (channel->hasClient(target))
		return _sendMessage(clientFd, ":ircserv 441 " + client->getNickname() + " " + target->getNickname() + " " + channel->getName() + " :They aren't on that channel");
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

void Server::_mode(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, channelName, flag, parameters;

	ss >> cmd >> channelName >> flag >> parameters;
	//if (channelName.empty())
	//	return _sendMessage(clientFd, ":ircserv 461 " + client->getNickname() + " MODE :Not enough parameters\r\n");
	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ":ircserv 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n");
	Channel *channel = _channels[channelName]; 
	//if (!channel->hasClient(client))
	//	return _sendMessage(clientFd, ":ircserv 442 " + client->getNickname() + " " + channelName + " :You're not on that channel");	
	if (flag.empty())
		return _sendMessage(clientFd, ":ircserv 324 "+ client->getNickname() + " " + channelName + " " + channel->getModes() + "\r\n");
	if (flag[1] == 'o')
		_modeO(client, clientFd, flag, parameters, channel);
	if (flag[1] == 'i')
		_modeI(channel, flag);

}