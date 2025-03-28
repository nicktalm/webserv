/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/27 12:58:16 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"

Server::Server(t_config config) : _config(config)
{
	addrinfo			hints;
	std::stringstream	tmp;
	int					yes = 1;

	std::cout << GREEN << "Starting server on port " << config.port << RESET << std::endl;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE; // fill in my IP for me

	tmp << config.port;
	if (auto status = getaddrinfo(config.server_name.c_str(), tmp.str().c_str(), &hints, &_res) != 0)
	{
		std::stringstream	error;

		error << "getaddrinfo error: " << gai_strerror(status);
		throw std::runtime_error(error.str());
	}

	_socketFd = socket(_res->ai_family, _res->ai_socktype, _res->ai_protocol);
	if (_socketFd == -1)
		throw std::runtime_error("socket failed");
	fcntl(_socketFd, F_SETFL, O_NONBLOCK);

	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
		throw std::runtime_error("setsockopt failed");

	if (bind(_socketFd, _res->ai_addr, _res->ai_addrlen) < 0)
		throw std::runtime_error("bind failed");

	if (listen(_socketFd, 10) < 0)
		throw std::runtime_error("listen failed");

	_clientsFd.push_back({_socketFd, POLLIN, 0});
}

Server::~Server(void)
{
	close(_socketFd);
	freeaddrinfo(_res);
}

void	Server::run(void)
{
	int eventCount = poll(_clientsFd.data(), _clientsFd.size(), 0);
	if (eventCount < 0)
		throw std::runtime_error("Poll failed");
	else if (eventCount > 0)
	{
		for (size_t it = 0; it < _clientsFd.size();)
		{
			if (_clientsFd[it].revents & POLLHUP) //client disconnected or hung up
			{
				std::cout << RED << "Client disconnected or hung up" << RESET << std::endl;
				close(_clientsFd[it].fd);
				_clientsFd.erase(_clientsFd.begin() + it);
				continue;
			}
			else if (_clientsFd[it].revents & POLLIN) //data ready to read
				this->request(_clientsFd[it].fd);
			else if (_clientsFd[it].revents & POLLOUT) //data ready to write
			{
				int	tmpFd = _clientsFd[it].fd;
				this->response(_clientsInfo[tmpFd]);
			}
			++it;
		}
	}
}

void Server::request(int fd)
{
	char		tmp[1024];

	if (fd == _socketFd) //new client trys to connect
	{
		int clientFd = accept(_socketFd, _res->ai_addr, &_res->ai_addrlen);
		if (clientFd < 0)
			perror("Client accept failed");
		else
		{
			std::cout << BLUE << "New client connected: " << clientFd << RESET << std::endl;
			fcntl(clientFd, F_SETFL, O_NONBLOCK);
			_clientsFd.push_back({clientFd, POLLIN, 0});
			_clientsInfo.insert(std::pair<int, Client>(clientFd, Client()));
		}
	}
	else //existing client trys to connect
	{
		int bytesRead = recv(fd, tmp, sizeof(tmp), 0);
		if (bytesRead <= 0)
			std::cerr << RED << "recv failed or client closed" << RESET << std::endl;
		else
		{
			_clientsInfo[fd].appendMsg(tmp, bytesRead);
			if (bytesRead < 1024)
			{
				_clientsInfo[fd].parseRequest(fd);
				_clientsInfo[fd].clearMsg();
			}
		}
	}
}

void	handleERROR(Client &client);

void	Server::response(Client &client)
{
	// if (client.getstatusCode() != "200")
	// 	handleERROR(client);
	if (client.getMethod() == "GET")
		handleGET(client);
	// else if(client.getMethod() == "POST")
	// 	handlePOST(&client);
	// else if(client.getMethod() == "DELETE")
	// 	handleDELETE(&client);
	// else
	// {
			
	// }
}

// void	handleERROR(Client &client)
// {
// 	std::string	response;
	
// 	response = client.getProtocol() + " " + client.getstatusCode() + " " + "NOT FOUND";
// 	send(client.getFd(), response.c_str(), )
// 	std::cout << response << std::endl;
// }

void	Server::handleGET(Client &client)
{
	std::string response;
	time_t currentTime;
	struct tm* ti;

	time(&currentTime);
	ti = localtime(&currentTime);
	
	//start line
	response = client.getProtocol() + " " + client.getstatusCode() + "Ok\n"; //Ok only if status is 200

	//Headers
	response += "Server: " + this->_config.server_name + "\n";
	response += "Date: " + std::string(asctime(ti));
	// response += "Content-Length: " + client.;
	response += "Content-Type: " + this->_config.server_name;

	//Body


	//Empty line
	response += "\n";

	//Body
}