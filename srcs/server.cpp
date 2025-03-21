/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/21 14:45:53 by lglauch          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"
#include <sys/socket.h>

Server::Server(t_config config) : _config(config)
{
	std::cout << GREEN << "Starting server on port " << config.port << RESET << std::endl;

	_socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFd == -1)
		throw std::runtime_error("socket failed");
	
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

	_clientsInfo.push_back({_socketFd, POLLIN, 0});
}

Server::~Server(void)
{
	// close(_socketFd);
}

void	Server::run(void)
{
	auto eventCount = poll(_clientsInfo.data(), _clientsInfo.size(), 0);
	if (eventCount < 0)
		std::cout << RED << "Poll failed" << RESET << std::endl;
	else if (eventCount > 0)
	{
		for (auto info = _clientsInfo.begin(); info != _clientsInfo.end();)
		{
			if (info->revents & POLLIN) //data ready to read
			{
				this->request(*info);
			}
			else if (info->revents & POLLOUT) //data ready to write
				this->response();
			else if (info->revents & POLLHUP) //client disconnected or hung up
			{
				std::cout << RED << "Client disconnected or hung up" << RESET << std::endl;
				close(info->fd);
				info = _clientsInfo.erase(info);
				continue;
			}
			else
			{
				std::cout << "No event catched?!" << std::endl; //mabye delete, idk if usefull
			}
			++info;
		}
	}
}

void Server::request(pollfd info){
	char		requestMsg[10000];

	if (info.fd == _socketFd)//new client trys to connect
	{
		socklen_t len = sizeof(_addr);
		int clientFd = accept(_socketFd, (struct sockaddr *)&_addr, &len);
		if (clientFd <= 0)
		{
			perror("Client accept failed");
		}
		else
		{
			std::cout << BLUE << "New client connected: " << clientFd << std::endl;
			_clientsInfo.push_back({clientFd, POLLIN, 0});
		}
	}
	else //existing client trys to connect
	{
		int bytesRead = recv(_socketFd, requestMsg, strlen(requestMsg), 0);
		if (bytesRead <= 0)
		{
			perror("Client recv empty or failed");
		}
	}
}


// void	Server::response(void)
// {
// 	std::string	html_page = readFile("index.html");

// 	std::string http_response =
// 		"HTTP/1.1 200 OK\r\n"
// 		"Content-Type: text/html\r\n"
// 		"Content-Length: " + std::to_string(html_page.size()) + "\r\n"
// 		"Connection: close\r\n"
// 		"\r\n" +
// 		html_page;

// 	int bytesSend = send(this->client, http_response.c_str(), http_response.length(), 0); //muessen poll dazu beutzen sonst grade 0
// 	if (bytesSend == 0)
// 		throw std::runtime_error("send is empty");
// 	else if (bytesSend < 0)
// 		throw std::runtime_error("send failed");
// }

std::string	readFile(std::string input)
{
	std::ifstream		file(input);
	std::stringstream	buffer;

	if (!file)
		throw std::runtime_error("open failed");
	buffer << file.rdbuf();
	return (buffer.str());
}