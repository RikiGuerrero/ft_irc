#include "Server.hpp"

void Server::_sendMessage(int fd, const std::string &msg)
{
	send(fd, msg.c_str(), msg.length(), 0);
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
