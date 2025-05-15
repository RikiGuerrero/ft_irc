#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _authenticated(false), _hasPass(false), _hasNick(false), _hasUser(false)
{}

Client::~Client()
{}

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
bool Client::isAuthenticated() const
{
    return _authenticated;
}
bool Client::hasAllInfo() const
{
    return _hasPass && _hasNick && _hasUser;
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
