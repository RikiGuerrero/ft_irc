#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <map>

class Client;

class Channel
{
	private:
		std::string _name;
		std::string _topic;
		bool _topicOpMode;//true solo op pueden cambiar
		std::set<Client *> _clients;
		std::set<Client *> _operators;
		std::map<int, Client *> _invited;//clientes invitados al canal

	public:
		Channel(const std::string &name);
		~Channel();

		const std::string &getName() const;
		const std::string &getTopic() const;
		const std::string &getTopic() const;
		const bool getTopicOpMode() const;

		void addClient(Client *client);
		void addInvited(Client *client);
		void addOperator(Client *client);

		void removeClient(Client *client);

		bool hasClient(Client *client) const;
		bool isOperator(Client *client) const;

		void setTopicOpMode(bool mode);
		void setTopic(const std::string &topic);
};

#endif
