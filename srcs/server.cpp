/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/20 12:15:57 by lglauch          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"

Server::Server(t_config config) : _config(config)
{
	std::cout << GREEN << "Starting server on port " << config.port << RESET << std::endl;

	_socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFd == -1)
		throw std::runtime_error("socket failed");

	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = INADDR_ANY;
	_addr.sin_port = htons(_config.port);

	if (bind(_socketFd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0)
		throw std::runtime_error("bind failed");

	_clientsInfo.push_back({_socketFd, POLLIN, 0});
}

Server::~Server(void)
{
	// close(_socketFd);
}

void	Server::request(void)
{
	socklen_t	len;
	char		requestMsg[10000];
	int			bytesRead;
	int			eventCount;
	int			clientFd;

	if (listen(_socketFd, 10) < 0)
	{
		perror("");
		throw std::runtime_error("listen failed");
	}

	eventCount = poll(_clientsInfo.data(), _clientsInfo.size(), -1);
	if (eventCount < 0)
		throw std::runtime_error("poll failed");
	
	if (eventCount > 0)
	{
		for (auto info : _clientsInfo)
		{
			if (info.fd == _socketFd)
			{
				if (info.revents & POLLIN)
				{
					std::cout << "new client" << std::endl;
					clientFd = accept(info.fd, (struct sockaddr *)&_addr, &len);
					std::cout << "client fd = " << clientFd << std::endl;
					if (clientFd < 0)
						throw std::runtime_error("accept failed");
					_clientsInfo.push_back({clientFd, POLLIN, 0});

					std::cout << "client send" << std::endl;
					bytesRead = recv(clientFd, requestMsg, 10000, 0);
					if (bytesRead < 0)
						throw std::runtime_error("recv failed");
					else if (bytesRead == 0)
						throw std::runtime_error("recv empty - client closed connection");
					std::cout << "Msg:" << std::endl;
					std::cout << requestMsg << std::endl;
				}
			}
			else
			{
				std::cout << "clientFd = " << info.fd << std::endl;
			}
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