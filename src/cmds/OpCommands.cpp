#include "Server.hpp"
#include "IrcMessages.hpp"

void Server::_topic(Client *client, int clientFd, const std::string &msg)//NO TESTEADO
{
	std::istringstream ss(msg);
	std::string cmd, channelName, newTopic;
	//checkar si esta en modo de operator
	ss >> cmd >> channelName;
	std::getline(ss, newTopic);
	if (channelName.empty())
		return _sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "TOPIC"));	
	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
	Channel *channel = _channels[channelName];
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channelName));	
	if (newTopic.empty()) //solo mirar el topico
	{
		if (channel->getTopic().empty())
			return _sendMessage(clientFd, RPL_NOTOPIC(client->getNickname(), channelName));
		else 
			return _sendMessage(clientFd, RPL_TOPIC(client->getNickname(), channelName, channel->getTopic()));
	}
	else if (channel->getTopicOpMode() == true && !channel->isOperator(client))//si solo puede cambiar los operadores y el cliente no lo es
		return _sendMessage(clientFd, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
	else
	{
		if (newTopic == ":")
		{
			channel->setTopic("");
			_broadcastToChannel(channelName,  "Clearing the topic on " + channelName + "\r\n", -1);//no se si hace falta ircserv
		}
		
		else
		{
			channel->setTopic(newTopic);
			_broadcastToChannel(channelName,  "Setting the topic on " + channelName + " to " + newTopic + "\r\n", -1);
		}
	}

}

void Server::_invite(Client *client, int clientFd, const std::string &msg)//NO TESTEADO
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
	_sendMessage(clientFd, RPL_INVITING(msg));//mensaje a quien invito 
	_sendMessage(target->getFd(), client->getNickname() + " has invited you to the channel " + channelName + "\r\n");
}

void Server::_kick(Client *client, int clientFd, const std::string &msg)//NO TESTEADO
{	
	std::istringstream ss(msg);
	std::string cmd, channelName, user, reason;
	ss >> cmd >> channelName >> user >> reason;

	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
	Channel *channel = _channels[channelName];
	if (channelName.empty() || user.empty())
		return _sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "TOPIC"));
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channelName));
	
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
		return _sendMessage(clientFd, ERR_USERNOTINCHANNEL(client->getNickname(), user, channelName));
	if (reason.empty())
		reason = "being too boring";
	_broadcastToChannel(channelName, target->getNickname() + " was kicked from " + channelName + " due to " + reason + "\r\n");
	_removeClient(target->getFd());
}