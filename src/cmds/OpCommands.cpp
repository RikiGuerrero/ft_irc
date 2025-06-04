#include "Server.hpp"
#include "IrcMessages.hpp"

void Server::_topic(Client *client, int clientFd, const std::string &msg)
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
	_sendMessage(clientFd, RPL_INVITING(msg));//mensaje a quien invito 
	_sendMessage(target->getFd(), client->getNickname() + " has invited you to the channel " + channelName + "\r\n");
}

void Server::_kick(Client *client, int clientFd, const std::string &msg)
{
	std::istringstream ss(msg);
	std::string cmd, channelName, reason, rest, lastUser, user;
	std::vector<std::string> users;
	
	ss >> cmd >> channelName >> rest;
	
	while (std::getline(ss, rest, ','))
	{
		if (!rest.empty() && rest[0] == ' ')
        	rest = rest.substr(1); // Remove espaço inicial
		std::cout << rest << "\n";
		users.push_back(rest);
	}
	
	std::string last = users.back();
	std::cout << last << "\n";
	std::istringstream sss(last);
	sss >> lastUser >> reason;
	std::cout << lastUser << " " << reason << std::endl;

	if (_channels.find(channelName) == _channels.end())
		return _sendMessage(clientFd, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
	Channel *channel = _channels[channelName];
	if (channelName.empty() || users.empty())
		return _sendMessage(clientFd, ERR_NEEDMOREPARAMS(client->getNickname(), "TOPIC"));
	if (!channel->isOperator(client))
		return _sendMessage(clientFd, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
	if (!channel->hasClient(client))
		return _sendMessage(clientFd, ERR_NOTONCHANNEL(client->getNickname(), channelName));

	for (std::vector<std::string>::iterator vit = users.begin(); vit != users.end(); vit++)
	{	
		Client *target = NULL;//encontra el target del mensaje
		for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			if (it->second->getNickname() == *vit)
			{
				target = it->second;
				break;
			}
		}
		std::cout << *vit << "\n";
		if (!target)
			_sendMessage(clientFd, ERR_USERNOTINCHANNEL(client->getNickname(), *vit, channelName));
		else if (client->getNickname() == *vit)
			_sendMessage(clientFd, ERR_NOSUCHNICK(client->getNickname(), *vit));
		else
		{
			std::string kickMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost ";
			if (reason.empty())
				reason = "being too boring";
			_broadcastToChannel(channelName, kickMsg + msg + "\r\n");
			_sendMessage(target->getFd(), target->getNickname() + " was kicked from " + channelName + " due to " + reason + "\r\n");
			channel->removeClient(target);
		}
	}
}