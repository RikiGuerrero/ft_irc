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
		std::string _pass;
		int _limit;
		bool _topicOpMode;
		bool _inviteOnly;
		bool _keyNeed;
		std::set<Client *> _clients;
		std::set<Client *> _operators;
		std::map<int, Client *> _invited;

	public:
		Channel(const std::string &name);
		~Channel();

		const std::string &getName() const;
		const std::string &getTopic() const;
		const std::string getModes() const;
		const std::string &getPass() const;
		int getLimit() const;
		bool getTopicOpMode() const;
		bool getInviteOlny() const;
		bool getKeyNeed() const;
		int getTotalUsers() const;
		bool hasClient(Client *client) const;
		bool isOperator(Client *client) const;
		bool isInvited (int clientFd) const;


		void addClient(Client *client);
		void addInvited(Client *client);
		void addOperator(Client *client);

		void removeClient(Client *client);
		void removeOperator(Client *client);


		void setTopicOpMode(bool mode);
		void setTopic(const std::string &topic);
		void setInviteOnly(bool mode);
		void setLimit(int limit);
		void setPass(const std::string &pass, bool flag);
};

#endif
