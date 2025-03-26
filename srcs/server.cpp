/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/26 17:45:33 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"

Server::Server(t_config config) : _config(config)
{
	std::cout << GREEN << "Starting server on port " << config.port << RESET << std::endl;
	
	memset(&_hints, 0, sizeof _hints);
	_hints.ai_family = AF_UNSPEC; // don't care IPv4 or IPv6
	_hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	_hints.ai_flags = AI_PASSIVE; // fill in my IP for me
	std::string port = std::to_string(config.port);
	const char *c_port = port.c_str();

	if (auto status = getaddrinfo(config.server_name.c_str(), c_port, &_hints, &_res) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		throw std::runtime_error("Addressinfo failed");
	}
	
	_socketFd = socket(_res->ai_family, _res->ai_socktype, _res->ai_protocol);
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

	if (bind(_socketFd, _res->ai_addr, _res->ai_addrlen) < 0)
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
				_clientsInfo.insert(std::pair<int, Client>(fd, Client(_clientsMsg[fd])));
				_clientsMsg[fd].clear();
			}
		}
	}
}

void	Server::response(const Client &client)
{
	if(client.getstatusCode() != "200")
		handleERROR(&client);
	if(client.getMethod() == "GET")
		handleGET(&client);
	else if(client.getMethod() == "POST")
		handlePOST(&client);
	else if(client.getMethod() == "DELETE")
		handleDELETE(&client);
	else
	{
			
	}
}