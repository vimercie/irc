/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmajani <mmajani@student.42lyon.fr>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/09/25 00:08:59 by vimercie          #+#    #+#             */
/*   Updated: 2023/12/09 18:55:30 by mmajani          ###   ########lyon.fr   */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Channel.hpp"
#include "../inc/Server.hpp"
#include "../inc/Client.hpp"
#include "../inc/IRCmsg.hpp"

Channel::Channel(std::string name) : name(name)
{
	if (name.length() > CHANNEL_NAME_MAX_LENGTH)
		this->name = name.substr(0, CHANNEL_NAME_MAX_LENGTH);

	if (name[0] != '#')
		this->name = "#" + this->name;

	std::cout << "Channel " + this->name + " created" << std::endl;
}

Channel::~Channel()
{
	for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); it++)
		delete *it;
	clients.clear();

	std::cout << "Channel " + name + " destroyed" << std::endl;
}

void	Channel::addClient(Client* client)
{
	clients.push_back(client);
}

std::string	Channel::getName(void) const
{
	return name;
}

std::string	Channel::getNamesList(void)
{
	std::string	result;

	for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); it++)
		result += (*it)->getNickname() + " ";

	return result;
}

std::vector<Client*>	Channel::getClients(void) const
{
	return clients;
}