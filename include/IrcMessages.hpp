#ifndef IRCMESSAGES_HPP
#define IRCMESSAGES_HPP

// Error messages
#define ERR_NOSUCHNICK(nick, target) 			":ft.irc 401 " + nick + " " + target + " :No such nick/channel\r\n"
#define ERR_NOSUCHCHANNEL(nick, channel) 		":ft.irc 403 " + nick + " " + channel + " :No such channel\r\n"
#define ERR_NICKNAMEINUSE(nick) 				":ft.irc 433 " + nick + " :Nickname is already in use\r\n"
#define ERR_NOTONCHANNEL(nick, channel) 		":ft.irc 442 " + nick + " " + channel + " :You're not on that channel\r\n"
#define ERR_NOTREGISTERED 						":ft.irc 451 :You have not registered\r\n"
#define ERR_NEEDMOREPARAMS(nick, command) 		":ft.irc 461 " + nick + " " + command + " :Not enough parameters\r\n"
#define ERR_ALREADYREGISTRED 					":ft.irc 462 :You may not reregister\r\n"
#define ERR_PASSWDMISMATCH 						":ft.irc 464 :Password incorrect\r\n"
#define ERR_CHANOPRIVSNEEDED(client, channel)	":ft.irc 482 " + client + " " + channel + " :You're not channel operator\r\n"
#define ERR_USERNOTINCHANNEL(client, target, channel)	":ft.irc 441 "  + client + " " + target + " " + channel + " :They aren't on that channel\r\n"
#define ERR_USERONCHANNEL(client, user, channel) ":ft.irc 443 " + client + " " + user + " " + channel + " is already on channel\r\n"
#define ERR_NORECIPIENT(client, cmd)			":ft.irc 411 " + client + " :No recipient given (" + cmd + ")\r\n"
#define ERR_BADCHANNELKEY(client, channel)	":ircserv 475" + client + " " + channel + " :Cannot join channel (+k)\r\n"
#define ERR_CHANNELISFULL(client, channel)	":ircserv 471" + client + " " + channel + " :Cannot join channel (+l)\r\n"
#define ERR_INVITEONLYCHAN(client, channel)	":ircserv 473" + client + " " + channel + " :Cannot join channel (+i)\r\n"



// Numerics
#define RPL_WELCOME(nick, username, hostname) 	":ft.irc 001 " + nick + " :Welcome to the ft_irc Network, " + nick + "!" + username + "@" + hostname + "\r\n"
#define RPL_NAMREPLY(nick, channel)				":ft.irc 353 " + nick + " = " + channel + " :"
#define RPL_ENDOFNAMES(nick, channel) 			":ft.irc 366 " + nick + " " + channel + " :End of NAMES list\r\n"
#define RPL_CHANNELMODES(client, channel, modes) ":ft.irc 324 " + client + " " + channel + modes + "\r\n"
#define RPL_NOTOPIC(client,  channel)			":ft.irc 331 " + client + " " + channel + " :No topic is set\r\n"
#define RPL_TOPIC(client,  channel, topic)		":ft.irc 332 " + client + " " + channel + " :" + topic + "\r\n"
#define RPL_INVITING(msg)						":ft.irc 341 " + msg + "\r\n" 

// Command messages
#define RPL_PONG(token) 						"PONG ft.irc " + token + "\r\n"
#define RPL_JOIN(nick, username, host, channel) ":" + nick + "!" + username + "@" + host + " JOIN :" + channel + "\r\n"
#endif