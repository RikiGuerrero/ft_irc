#include "Client.hpp"

Client::Client(int fd, const std::string &hostname) : _fd(fd), _authenticated(false), _hostname(hostname), _hasPass(false), _hasNick(false), _hasUser(false)
{}

Client::~Client()
{}

std::string &Client::getRecvBuffer()
{
    return _recvBuffer;
}

void Client::appendToBuffer(const std::string &data)
{
    _recvBuffer.append(data);
}

int Client::getFd() const
{
    return _fd;
}
const std::string &Client::getNickname() const
{
    return _nickname;
}
const std::string &Client::getUsername() const
{
    return _username;
}
const std::string &Client::getHostname() const
{
   return _hostname; 
}
bool Client::isAuthenticated() const
{
    return _authenticated;
}
bool Client::hasAllInfo() const
{
    return _hasPass && _hasNick && _hasUser;
}
bool Client::isRegistered() const
{
    return _hasNick && _hasUser;
}
bool Client::isPassSet() const
{
    return _hasPass;
}
void Client::setNickname(const std::string &nickname)
{
    _nickname = nickname;
    _hasNick = true;
}
void Client::setUsername(const std::string &username)
{
    _username = username;
    _hasUser = true;
}
void Client::setRealname(const std::string &realname)
{
    _realname = realname;
}
void Client::setPass(bool ok)
{
    _hasPass = ok;
}
void Client::tryAuthenticate()
{
    if (hasAllInfo() && !_authenticated)
        _authenticated = true;
}
std::string Client::getPrefix() const
{
    std::ostringstream oss;
    oss << ":" << _nickname << "!" << _username << "@" << _hostname;
    return oss.str();
}
