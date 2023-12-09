/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: vimercie <vimercie@student.42lyon.fr>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/11/15 18:16:36 by vimercie          #+#    #+#             */
/*   Updated: 2023/12/09 17:05:47 by vimercie         ###   ########lyon.fr   */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/IRCmsg.hpp"
#include "../inc/Channel.hpp"
#include "../inc/Client.hpp"
#include "../inc/Utils.hpp"

int	status = 1;
void statusHandler(int sig);

Server::Server(int port, const std::string& password) : port(port), password(password)
{
	std::cout << "Server created" << std::endl;
	initialize();
}

Server::~Server()
{
	// Fermeture du socket serveur
	close(sockfd);

	// Suppression des channels
	for (std::vector<Channel*>::iterator it = channels.begin(); it != channels.end(); it++)
		delete *it;
	channels.clear();

	std::cout << "Server destroyed" << std::endl;
}

void Server::initialize()
{
	// Ignorer sigpipe
	signal(SIGPIPE, SIG_IGN);
	// Gérer SIGINT
	signal(SIGINT, statusHandler);
	// Création du socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		std::cerr << "Erreur de création du socket" << std::endl;
		exit(1);
	}

	// Configuration de l'adresse du serveur
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(this->port);

	// Liaison du socket à l'adresse du serveur
	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
	{
		std::cerr << "Erreur de liaison du socket" << std::endl;
		exit(1);
	}

	// Mise en écoute du serveur
	if (listen(sockfd, 5) != 0)
	{
		std::cerr << "Erreur lors de la mise en écoute" << std::endl;
		exit(1);
	}

	// Initialisation du poll
	for (int i = 0; i < MAX_CLIENTS; i++)
		fds[i].fd = -1;

	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	nfds = 1;
	std::cout << "Serveur lancé sur le port " << port << std::endl;

	name = "localhost";

	// Création du channel par défaut
	channels.push_back(new Channel("General"));
}

void	Server::serverLoop()
{
	int	poll_ret;

	while (status)
	{
		poll_ret = poll(fds, nfds, 0);

		if (poll_ret < 0 && status == 1)
		{
			std::cerr << "Erreur de poll" << std::endl;
			break;
		}

		if (fds[0].revents & POLLIN)
			acceptConnections();

		communicate();
	}
}

void Server::acceptConnections()
{
	struct sockaddr_in	cli;
	socklen_t			len = sizeof(cli);
	int					connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	int					sockIndex = 1;

	if (connfd < 0)
	{
		std::cerr << "Erreur de connexion" << std::endl;
		return;
	}

	std::cout << "Nouvelle connexion acceptée" << std::endl;

	while (fds[sockIndex].fd != -1 && sockIndex < MAX_CLIENTS)
		sockIndex++;

	if (sockIndex == MAX_CLIENTS)
	{
		std::cout << "Trop de clients connectés" << std::endl;
		return;
	}

	fds[sockIndex].fd = connfd;
	fds[sockIndex].events = POLLIN;
	nfds++;
	addClient(&fds[sockIndex]);
}

void	Server::communicate()
{
	std::vector<IRCmsg*>	messages;

	for (nfds_t i = 1; i < nfds; i++)
	{
		// Si le socket est libre
		if (fds[i].fd == -1)
			continue;

		// Si le socket a une erreur
		if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
		{
			close(fds[i].fd);
			fds[i].fd = -1;
			continue;
		}

		// Si le socket est prêt à être lu
		if (fds[i].revents & POLLIN)
		{
			messages = readMsg(fds[i].fd);

			// Lecture des messages entrants
			for (std::vector<IRCmsg*>::iterator it = messages.begin(), end = messages.end(); it != end; it++)
			{
				if ((*it)->getCommand().empty())
					continue;
				std::cout << (*it)->toString();
			}
		}
	}
}

std::vector<IRCmsg*>	Server::readMsg(int fd)
{
	char					buffer[1024];
	ssize_t					bytes_read;
	std::vector<IRCmsg*>	res;

	memset(buffer, 0, sizeof(buffer));

	bytes_read = recv(fd, buffer, sizeof(buffer), MSG_DONTWAIT);

	// Si on a reçu des données
	if (bytes_read > 0)
	{
		std::vector<std::string>	msgs = splitString(buffer, "\r\n");

		for (std::vector<std::string>::iterator it = msgs.begin(); it != msgs.end(); it++)
			res.push_back(new IRCmsg(getClientByFd(fd), *it));
	}
	// Si le client s'est déconnecté
	else if (bytes_read == 0)
	{
		std::cout << "Client déconnecté." << std::endl;
		close(fd);
		fds[fd].fd = -1;  // Marquer comme libre
	}
	// Si on a une erreur
	else if (errno != EWOULDBLOCK)		// CONDITION INTERDITE (À supprimer) (utiliser fcntl(fd, F_SETFL, O_NONBLOCK) à la place ?)
		std::cerr << "Erreur de lecture du socket client." << std::endl;

	return res;
}

void	Server::sendMsg(int fd, const std::string& msg)
{
	ssize_t	bytes_sent;

	bytes_sent = send(fd, msg.c_str(), msg.length(), 0);

	if (bytes_sent == -1)
		std::cerr << "Erreur d'envoi du message" << std::endl;
}

void	Server::addClient(pollfd *socket)
{
	Client* newClient = new Client(socket);

	// Tant que le client n'a pas envoyé les infos de base
	while (newClient->getNickname().empty()
		|| newClient->getUsername().empty()
		|| newClient->getHostname().empty()
		|| newClient->getRealname().empty())
	{
		std::vector<IRCmsg*>	messages = readMsg(newClient->getSocket().fd);

		// On parcourt les messages reçus (un ou plusieurs messages peuvent être reçus en même temps dans le buffer)
		for (std::vector<IRCmsg*>::iterator it = messages.begin(); it != messages.end(); it++)
		{
			if ((*it)->getCommand().empty())
				continue;

			// Exécution des commandes
			newClient->execCmd(*(*it));
		}
	}

	clients.push_back(newClient);
	channels[0]->addClient(newClient);

	std::cout << "Client " << newClient->getNickname() << " connecté" << std::endl;

	welcome(newClient);
}

Client*	Server::getClientByFd(int fd)
{
	for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); it++)
		if ((*it)->getSocket().fd == fd)
			return *it;
	return NULL;
}

void	Server::welcome(Client* client)
{
	IRCmsg						msg;
	std::vector<std::string>	params;

	msg.setPrefix(name);

	msg.setCommand("001");

	params.push_back(client->getNickname());
	msg.setParameters(params);

	msg.setTrailing("Wesh wesh wesh " + client->getNickname());

	sendMsg(client->getSocket().fd, msg.toString());
}
