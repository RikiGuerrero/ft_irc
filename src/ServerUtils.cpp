#include "Server.hpp"

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
	const std::string &user = client->getUsername();

	_sendMessage(clientFd, ":ircserv 001 " + nick + " :Welcome to the IRC server " + nick + "!" + user + "@localhost\r\n");
	_sendMessage(clientFd, ":ircserv 002 " + nick + " :Your host is ircserv, running version 1.0\r\n");
	_sendMessage(clientFd, ":ircserv 003 " + nick + " :This server was created just now\r\n");
	_sendMessage(clientFd, ":ircserv 376 " + nick + " :End of MOTD command\r\n");
}

void Server::_broadcastToChannel(const std::string &channelName, const std::string &msg, int excludeFd)
{
	Channel* channel = _channels[channelName];//busca el canal deseado y envia mensaje
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