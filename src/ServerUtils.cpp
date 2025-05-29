#include "Server.hpp"
#include "IrcMessages.hpp"

void Server::_sendMessage(int fd, const std::string &msg)
{
	send(fd, msg.c_str(), msg.length(), 0);//envia un mensage a un fd
}

bool Server::_nicknameExists(const std::string &nickname)
{
	for (std::map<int, Client *>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->second->getNickname() == nickname)
			return true;
	}
	return false;
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

/* //INCOMPLETO
std::string Server::getError(int error, std::string name,  std::string sec_part)//sec_part puede ser nombre del canal o target
{
	std::stringstream ss;
	ss << ":ircserv " << error << " ";
	
	std::map<int, std::string> errors;
	errors[403] = name + " " + sec_part + " :No such channel\r\n";
	errors[482] = name + " " + sec_part + " :You're not channel operator\r\n";
	errors[442] = name + " " + sec_part + " :You're not on that channel\r\n";

} */