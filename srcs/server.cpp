/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/26 09:49:01 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../include/server.hpp"

Server::Server(t_config config) : _config(config)
{
	std::cout << GREEN << "Starting server on port " << config.port << RESET << std::endl;

	_socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFd == -1)
		throw std::runtime_error("socket failed");
	
	fcntl(_socketFd, F_SETFL, O_NONBLOCK);
	std::cout << "client fd = " << _socketFd << std::endl;
	//gut, weil wenn der server restartet kann er den port direkt nutzen, weil der Kernel sonst beim closen bissl braucht bis man wieder dazu binden kann
	int yes = 1;
	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
	{
		perror("setsockopt");
		throw std::runtime_error("setsockopt failed");
	}
	
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = INADDR_ANY;
	_addr.sin_port = htons(_config.port);

	if (bind(_socketFd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0)
		throw std::runtime_error("bind failed");

	if (listen(_socketFd, 10) < 0)
	{
		perror("");
		throw std::runtime_error("listen failed");
	}

	_clientsFd.push_back({_socketFd, POLLIN, 0});
}

Server::~Server(void)
{
	close(_socketFd);
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
				this->response(_clientsFd[it].fd);
			++it;
		}
	}
}

void Server::request(int fd)
{
	char		tmp[1024];

	if (fd == _socketFd) //new client trys to connect
	{
		socklen_t len = sizeof(_addr);
		int clientFd = accept(_socketFd, (struct sockaddr *)&_addr, &len);
		if (clientFd < 0)
			perror("Client accept failed");
		else
		{
			std::cout << BLUE << "New client connected: " << clientFd << RESET << std::endl;
			fcntl(clientFd, F_SETFL, O_NONBLOCK);
			_clientsFd.push_back({clientFd, POLLIN, 0});
			_clientsMsg.insert(std::pair(clientFd, ""));
		}
	}
	else //existing client trys to connect
	{
		int bytesRead = recv(fd, tmp, sizeof(tmp), 0);
		if (bytesRead <= 0)
			std::cerr << RED << "recv failed or client closed" << RESET << std::endl;
		else
		{
			_clientsMsg[fd].append(tmp, bytesRead);
			if (bytesRead < 1024)
			{
				_clientsInfo.emplace_back(fd, _clientsMsg[fd]);
				_clientsMsg[fd].clear();
			}
		}
	}
}

void	Server::response(int fd)
{
	std::cout << "fd = " << fd << std::endl;
}