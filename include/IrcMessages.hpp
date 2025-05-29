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

// Numerics
#define RPL_WELCOME(nick, username, hostname) 	":ft.irc 001 " + nick + " :Welcome to the ft_irc Network, " + nick + "!" + username + "@" + hostname + "\r\n"
#define RPL_NAMREPLY(channel) 					":ft.irc 353 = " + channel + " : "
#define RPL_ENDOFNAMES(nick, channel) 			":ft.irc 366 " + nick + " " + channel + " :End of NAMES list\r\n"

// Command messages
#define RPL_PONG(token) 						"PONG ft.irc " + token + "\r\n"
#define RPL_JOIN(nick, username, host, channel) ":" + nick + "!" + username + "@" + host + " JOIN :" + channel + "\r\n"

#endif