/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ntalmon <ntalmon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/05/08 18:48:42 by ntalmon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <fstream>
#include <sys/fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <algorithm>
#include "../include/response.hpp"
#include "../include/utils.hpp"
#include "../include/server.hpp"

Server::Server(t_config config) : _config(config)
{
	addrinfo			hints;
	std::stringstream	tmp;
	int					yes = 1;
	std::string			costum_name = "";

	log(4, "Server created:", config.port);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me

	size_t pos = config.port.find(':');
	if (pos != std::string::npos)
	{
		costum_name = config.port.substr(0, pos);
		tmp << config.port.substr(pos + 1);
	}
	else
		tmp << config.port;
	config.port = tmp.str();
	int status = getaddrinfo(costum_name.c_str(), tmp.str().c_str(), &hints, &_res);
	if (status != 0)
	{
		std::stringstream	error;

		error << "getaddrinfo error: " << gai_strerror(status);
		throw std::runtime_error(error.str());
	}

	_socketFd = socket(_res->ai_family, _res->ai_socktype, _res->ai_protocol);
	if (_socketFd == -1)
		throw std::runtime_error("socket failed");
	if (fcntl(_socketFd, F_SETFL, O_NONBLOCK) == -1)
	{
		throw std::runtime_error("fcntl");
	}	

	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
		throw std::runtime_error("setsockopt failed");

	if (bind(_socketFd, _res->ai_addr, _res->ai_addrlen) < 0)
		throw std::runtime_error("bind failed");

	if (listen(_socketFd, SOMAXCONN) < 0)
		throw std::runtime_error("listen failed");

	_clientsFd.push_back({_socketFd, POLLIN, 0});
}

Server::~Server(void)
{
	for (auto it = _clientsFd.rbegin(); it != _clientsFd.rend(); ++it)
	{
		if (it.base() != _clientsFd.end())
			this->disconnect(it.base());
	}
	log(0, "Server closed:", _config.server_name.c_str());
	close(_clientsFd.begin()->fd);
	_clientsFd.erase(_clientsFd.begin());
	freeaddrinfo(_res);
}

void	Server::run(void)
{
	int eventCount = poll(_clientsFd.data(), _clientsFd.size(), 0);
	if (eventCount < 0 && runner)
		throw std::runtime_error("Poll failed");
	else if (eventCount > 0)
	{
		for (size_t it = 0; it < _clientsFd.size();)
		{
			if (_clientsFd[it].revents & POLLHUP || _clientsFd[it].revents & POLLERR || _clientsFd[it].revents & POLLNVAL) //client disconnected or hung up
			{
				this->disconnect(_clientsFd.begin() + it);
				continue;
			}
			else if (_clientsFd[it].revents & POLLIN) //data ready to read
				this->request(_clientsFd.begin() + it);
			else if (_clientsFd[it].revents & POLLOUT) //data ready to write
			{
				int	tmpFd = _clientsFd[it].fd;
				this->response(_clientsInfo[tmpFd], _clientsFd.begin() + it);
			}
			++it;
		}
	}
}

std::string	Server::readFromFd(Client &client)
{
	ssize_t	bytesRead;
	char	buffer[8192];

	bytesRead = read(client.getCGIOutput(), buffer, sizeof(buffer));

	if (bytesRead == -1)
	{
		std::cerr << RED << "read failed" << RESET << std::endl;
		client.setStatusCode("500");
		client.setReady(true);
		client.handleFd(close, client.getCGIOutput());
		return ("");
	}
	else if (bytesRead == 0)
	{
		if (!client.handleFd(close, client.getCGIOutput()))
			return ("");
		client.setReady(true);
		return ("");
	}
	else
		buffer[bytesRead] = '\0';
	return (this->checkCGIOutput(client, buffer));
}

std::string	Server::checkCGIOutput(Client &client, char *buffer)
{
	std::string	tmp(buffer);
	size_t		pos;

	pos = tmp.find("Status:");
	if (pos != std::string::npos)
		tmp.replace(pos, pos + 7, "HTTP/1.1");
	else
	{
		client.setStatusCode("400");
		client.setReady(true);
		return ("");
	}
	pos = tmp.find("Content-Length:");
	if (pos == std::string::npos)
	{
		client.setStatusCode("400");
		client.setReady(true);
		return ("");
	}
	pos = tmp.find("Content-Type:");
	if (pos == std::string::npos)
	{
		client.setStatusCode("400");
		client.setReady(true);
		return ("");
	}
	return (tmp);
}

void	Server::response(Client &client, std::vector<pollfd>::iterator pollClient)
{
	if (client.getBytesSend() == static_cast<ssize_t>(client.getResponseBuffer().size()))
	{
		if (client.getStatusCode()[0] == '4' || client.getStatusCode()[0] == '5')
			client.setResponseBuffer(handleERROR(client));
		else if (client.getMethod() == "GET")
			client.setResponseBuffer(handleGET(client));
		else if(client.getMethod() == "POST")
			client.setResponseBuffer(handlePOST(client));
		else if(client.getMethod() == "DELETE")
			client.setResponseBuffer(handleDELETE(client));
		if (client.getStatusCode()[0] == '4' || client.getStatusCode()[0] == '5')
			client.setResponseBuffer(handleERROR(client));
	}

	std::string	response = client.getResponseBuffer();
	ssize_t		bytesSent = client.getBytesSend();
	ssize_t		remaining = response.size() - bytesSent;
	ssize_t		bytes = 0;

	// Send as much as possible
	if (!response.empty())
	{
		bytes = send(client.getFd(), response.c_str() + bytesSent, remaining, MSG_DONTWAIT);
		if (bytes <= 0)
		{
			std::cout << RED << "send failed" << RESET << std::endl;
			this->disconnect(pollClient);
			return;
		}
	}
	ssize_t	currBytes = bytesSent + bytes;
	client.setBytesSend(currBytes);
	// nachdem alles gesendet wurde, client aud POLLIN einstellen
	if (currBytes == static_cast<ssize_t>(response.size()))
	{
		client.setResponseBuffer(""); // Clear buffer fuer naechstes mal
		client.setBytesSend(0);
	}
	if (client.getReady() && (currBytes == static_cast<ssize_t>(response.size())))
	{
		pollClient->events = POLLIN;
		client.clear();
	}
}

std::string Server::create_response(const t_response &response)
{
	std::string finished;

	finished =
	response.start_line + "\r\n" +
	response.server_name + "\r\n" +
	response.date + "\r\n" +
	response.content_length + "\r\n" +
	response.content_type + "\r\n" +
	response.empty_line +
	response.body  + "\r\n";
	return (finished);
}

void	Server::disconnect(std::vector<pollfd>::iterator find)
{
	log(0, "Client disconnected:", find->fd);
	auto del = _clientsInfo.find(find->fd);
	if (del != _clientsInfo.end())
		_clientsInfo.erase(del);
	close(find->fd);
	_clientsFd.erase(find);
}
