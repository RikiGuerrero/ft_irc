#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string &name) : _name(name), _topic(""), _limit(-1), _topicOpMode(false),  _inviteOnly(false), _keyNeed(false) {}
Channel::~Channel() {}

const std::string &Channel::getName() const
{
	return _name;
}
const std::string &Channel::getTopic() const
{
	return _topic;
}

void Channel::addClient(Client *client)
{
	_clients.insert(client);
}
void Channel::removeClient(Client *client)
{
	if (isOperator(client))
		_operators.erase(client);
	_clients.erase(client);
}
bool Channel::hasClient(Client *client) const
{
	return _clients.find(client) != _clients.end();
}
bool Channel::isOperator(Client *client) const
{
	return _operators.find(client) != _operators.end();
}

void Channel::setTopic(const std::string &topic)
{
	_topic = topic;
}
void Channel::addOperator(Client *client)
{
	_operators.insert(client);
}

void Channel::addInvited(Client *client)
{
	if(client)
		_invited[client->getFd()] = client;
}

void Channel::setTopicOpMode(bool mode)
{
	_topicOpMode = mode;
}

bool Channel::getTopicOpMode() const
{
	return _topicOpMode;
}

const std::string Channel::getModes() const
{
	std::string result = "";
	if (_topicOpMode)
		result += " +t ";
	if (_inviteOnly)
		result += " +i ";
	if (_keyNeed)
		result += " +k ";
	return result;
}

void Channel::removeOperator(Client *client)
{
	_operators.erase(client);
}

void Channel::setInviteOnly(bool mode)
{
	_inviteOnly = mode;
}

void Channel::setLimit(int limit)
{
	_limit = limit;
}

void Channel::setPass(const std::string &pass, bool flag)
{
	_pass = pass;
	_keyNeed = flag;
}

const std::string &Channel::getPass() const
{
	return _pass;
}

bool Channel::getInviteOlny() const
{
	return _inviteOnly;
}

bool Channel::getKeyNeed() const
{
	return _keyNeed;
}

int Channel::getLimit() const
{
	return _limit;
}

int Channel::getTotalUsers() const
{
	return _clients.size();
}

bool Channel::isInvited(int clientFd) const
{
	if (_invited.find(clientFd) == _invited.end())
		return false;
	return true;
}