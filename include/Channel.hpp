#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Client;

class Channel
{
	private:
		std::string _name;
		std::string _topic;
		std::set<Client *> _clients;//hcaer como Map
		std::set<Client *> _operators;

	public:
		Channel(const std::string &name);
		~Channel();

		const std::string &getName() const;
		const std::string &getTopic() const;

		void addClient(Client *client);
		void removeClient(Client *client);
		bool hasClient(Client *client) const;
		bool isOperator(Client *client) const;

		void setTopic(const std::string &topic);
		void addOperator(Client *client);
};

#endif
