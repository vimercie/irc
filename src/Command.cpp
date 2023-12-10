/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vimercie <vimercie@student.42lyon.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/12/06 12:38:54 by mmajani           #+#    #+#             */
/*   Updated: 2023/12/10 02:54:46 by vimercie         ###   ########lyon.fr   */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Command.hpp"
#include "../inc/IRCmsg.hpp"
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"
#include "../inc/Utils.hpp"

Command::Command(const IRCmsg& msg, Client* client)
{
	userCmds["NICK"] = &Command::nick;
	userCmds["USER"] = &Command::user;

	exec(msg, client);
}

Command::~Command() {}

void	Command::exec(const IRCmsg& msg, Client* client)
{
	std::map<std::string, userCmd>::iterator	it = userCmds.find(msg.getCommand());

	if (it != userCmds.end())
		(this->*(it->second))(msg, client);
}

// int Command::exec(const IRCmsg& msg, Channel* channel)
// {
	
// }

// int Command::exec(const IRCmsg& msg, Server* server)
// {
	
// }


void	Command::nick(const IRCmsg& msg, Client* client)
{
	client->setNickname(msg.getParameters()[0]);
	welcome(client);
}

void	Command::user(const IRCmsg& msg, Client* client)
{
	client->setUsername(msg.getParameters()[0]);
	client->setHostname(msg.getParameters()[1]);
	client->setRealname(msg.getTrailing());
}

void	Command::welcome(Client* client)
{
	IRCmsg	msg;

	msg.setCommand("001");
	msg.setPrefix("localhost");
	msg.setParameters(std::vector<std::string>(1, client->getNickname()));
	msg.setTrailing("Wesh wesh wesh " + client->getNickname());

	sendMsg(client->getSocket().fd, msg.toString());
}
