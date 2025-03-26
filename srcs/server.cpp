/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/26 12:25:43 by lglauch          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../include/server.hpp"
#include <netdb.h>

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
	
	// _addr.sin_family = AF_INET;
	// _addr.sin_addr.s_addr = INADDR_ANY;
	// _addr.sin_port = htons(_config.port);

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
	freeaddrinfo(_res);
}

void	Server::run(void)
{
	int eventCount = poll(_clientsFd.data(), _clientsFd.size(), 0);
	if (eventCount < 0)
		std::cout << RED << "Poll failed" << RESET << std::endl;
	else if (eventCount > 0)
	{
		for (size_t it = 0; it < _clientsFd.size();)
		{
			if (_clientsFd[it].revents & POLLIN) //data ready to read
				this->request(_clientsFd[it].fd);
			else if (_clientsFd[it].revents & POLLOUT) //data ready to write
				this->response(_clientsFd[it].fd);
			if (_clientsFd[it].revents & POLLHUP) //client disconnected or hung up
			{
				std::cout << RED << "Client disconnected or hung up" << RESET << std::endl;
				close(_clientsFd[it].fd);
				_clientsFd.erase(_clientsFd.begin() + it);
				continue;
			}
			else
				++it;
		}
	}
}

void Server::request(int fd)
{
	char		tmp[1024];

	std::cout << "request" << std::endl;
	if (fd == _socketFd) //new client trys to connect
	{
		socklen_t len = sizeof(_addr);
		int clientFd = accept(_socketFd, (struct sockaddr *)&_addr, &len);
		if (clientFd <= 0)
		{
			perror("Client accept failed");
		}
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
		std::cout << BLUE << "msg from client " << fd << RESET << std::endl;
		int bytesRead = recv(fd, tmp, sizeof(tmp), 0);
		if (!bytesRead)
			std::cout << "nothing to read" << std::endl;
		else if (bytesRead < 0)
			perror("recv");
		else
		{
			_clientsMsg[fd].append(tmp, bytesRead);
			if (bytesRead < 1024)
			{
				_clientsInfo.emplace_back(Client(fd, _clientsMsg[fd]));
				_clientsMsg[fd].clear();
			}
		}
	}
}


void	Server::response(int fd)
{
	std::cout << "response " << fd << std::endl;
	std::string	html_page = readFile("index.html");

	// std::string http_response =
	// 	"HTTP/1.1 200 OK\r\n"
	// 	"Content-Type: text/html\r\n"
	// 	"Content-Length: " + std::to_string(html_page.size()) + "\r\n"
	// 	"Connection: close\r\n"
	// 	"\r\n" +
	// 	html_page;
	// std::string http_response =
	// 	"HTTP/1.1 200 OK\r\n"
	// 	"Content-Type: text/html\r\n"
	// 	"Content-Length: " + std::to_string(html_page.size()) + "\r\n"
	// 	"Connection: close\r\n"
	// 	"\r\n" +
	// 	html_page;

	// int bytesSend = send(this->client, http_response.c_str(), http_response.length(), 0); //muessen poll dazu beutzen sonst grade 0
	// if (bytesSend == 0)
	// 	throw std::runtime_error("send is empty");
	// else if (bytesSend < 0)
	// 	throw std::runtime_error("send failed");

	// int bytesSend = send(this->client, http_response.c_str(), http_response.length(), 0); //muessen poll dazu beutzen sonst grade 0
	// if (bytesSend == 0)
	// 	throw std::runtime_error("send is empty");
	// else if (bytesSend < 0)
	// 	throw std::runtime_error("send failed");
}

std::string	readFile(std::string input)
{
	std::ifstream		file(input);
	std::stringstream	buffer;

	if (!file)
		throw std::runtime_error("open failed");
	buffer << file.rdbuf();
	return (buffer.str());
}