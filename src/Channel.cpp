#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string &name) : _name(name), _topic("") {}
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
	_clients.erase(client);
	_operators.erase(client);
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
	_invited[client->getFd()] = client;
}

const std::string &Channel::getTopic() const
{
	return _topic;
}

void Channel::setTopicOpMode(bool mode)
{
	_topicOpMode = mode;
}

const bool Channel::getTopicOpMode() const
{
	return _topicOpMode;
}