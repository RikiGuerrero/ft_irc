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

    if (sendBuffer.empty()) {
        // Nada que enviar, desactivar POLLOUT
        for (std::size_t i = 0; i < _pollFds.size(); ++i) {
            if (_pollFds[i].fd == clientFd) {
                _pollFds[i].events &= ~POLLOUT; // Desactiva POLLOUT
                //std::cout << "Deactivated POLLOUT for FD " << clientFd << std::endl; // Debug
                return;
            }
        }
        return; // No debería llegar aquí si el cliente existe y su FD está en _pollFds
    }

    // Intentar enviar datos del búfer
    // sendBuffer.c_str() es C++98
    ssize_t bytesSent = send(clientFd, sendBuffer.c_str(), sendBuffer.length(), 0);

    if (bytesSent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // El búfer de envío del kernel está lleno temporalmente.
            // NO reintentamos aquí; poll() nos notificará de nuevo cuando esté listo.
            //std::cout << "Send blocked for FD " << clientFd << ". Will retry later." << std::endl; // Debug
            return;
        } else {
            // Error real de envío. Desconectar cliente.
            std::cerr << "Error sending to client " << clientFd << ": " << strerror(errno) << std::endl;
            _removeClient(clientFd);
            return;
        }
    } else if (bytesSent == 0) {
        // El cliente cerró la conexión en medio del envío
        std::cout << "Client " << clientFd << " closed connection during send." << std::endl;
        _removeClient(clientFd);
        return;
    } else {
        // Se enviaron algunos bytes, eliminarlos del búfer de envío del cliente
        client->eraseSendBuffer(static_cast<size_t>(bytesSent));
        //std::cout << "Sent " << bytesSent << " bytes to FD " << clientFd << ". "
        //          << client->getSendBuffer().length() << " bytes remaining." << std::endl; // Debug

        if (client->getSendBuffer().empty()) {
            // Si todo el búfer se envió, desactivar POLLOUT para evitar notificaciones innecesarias
            for (std::size_t i = 0; i < _pollFds.size(); ++i) {
                if (_pollFds[i].fd == clientFd) {
                    _pollFds[i].events &= ~POLLOUT; // Desactiva POLLOUT
                    //std::cout << "Deactivated POLLOUT for FD " << clientFd << " (buffer empty)." << std::endl; // Debug
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