#include "Server.hpp"
#include "IrcMessages.hpp"

void Server::_sendMessage(int fd, const std::string &msg)
{
	std::map<int, Client *>::iterator it = _clients.find(fd);
	if (it == _clients.end())
	{
        std::cerr << "Error: Attempted to queue message for non-existent client FD " << fd << std::endl;
        return;
    }
    Client* client = it->second;

    std::string fullMessage = msg;
    client->appendToSendBuffer(fullMessage);

    for (std::size_t i = 0; i < _pollFds.size(); ++i) {
        if (_pollFds[i].fd == fd) 
		{
            _pollFds[i].events |= POLLOUT;
            return;
        }
    }
    std::cerr << "Error: FD " << fd << " not found in _pollFds vector for queueing message." << std::endl;
}

void Server::_handleClientWrite(int clientFd) {
    std::map<int, Client *>::iterator it = _clients.find(clientFd);
    if (it == _clients.end())
        return;

    Client* client = it->second;

    std::string& sendBuffer = const_cast<std::string&>(client->getSendBuffer());

    if (sendBuffer.empty()) 
    {
        for (std::size_t i = 0; i < _pollFds.size(); ++i)
        {
            if (_pollFds[i].fd == clientFd) {
                _pollFds[i].events &= ~POLLOUT;
                return;
            }
        }
        return;
    }

    ssize_t bytesSent = send(clientFd, sendBuffer.c_str(), sendBuffer.length(), 0);

    if (bytesSent == -1) 
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        else
        {
            std::cerr << "Error sending to client " << clientFd << ": " << strerror(errno) << std::endl;
            _removeClient(clientFd);
            return;
        }
    } 
    else if (bytesSent == 0)
    {
        std::cout << "Client " << clientFd << " closed connection during send." << std::endl;
        _removeClient(clientFd);
        return;
    } 
    else
    {
        client->eraseSendBuffer(static_cast<size_t>(bytesSent));
        if (client->getSendBuffer().empty()) {
            for (std::size_t i = 0; i < _pollFds.size(); ++i) {
                if (_pollFds[i].fd == clientFd) {
                    _pollFds[i].events &= ~POLLOUT;
                    break;
                }
            }
        }
    }
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
