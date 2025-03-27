/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/27 17:43:52 by lglauch          ###   ########.fr       */
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
		throw std::runtime_error("setsockopt failed");
	}

	if (bind(_socketFd, _res->ai_addr, _res->ai_addrlen) < 0)
		throw std::runtime_error("bind failed");

	if (listen(_socketFd, 10) < 0)
	{
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
				std::cout << YELLOW << "Client disconnected" << RESET << std::endl;
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

void	Server::handleRecvError(int bytesRead, int fd)
{
	if (bytesRead == 0)
		std::cout << RED << "Client disconnected" << RESET << std::endl;
	else
		std::cout << RED << "Recv failed" << RESET << std::endl;
	close(fd);
	_clientsInfo.erase(fd);
	_clientsMsg.erase(fd);
	for (size_t i = 0; i < _clientsFd.size(); i++)
	{
		if (_clientsFd[i].fd == fd)
		{
			_clientsFd.erase(_clientsFd.begin() + i);
			break;
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
			std::cout << RED << "Client accept failed" << RESET << std::endl;
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
			handleRecvError(bytesRead, fd);
		else
		{
			_clientsMsg[fd].append(tmp, bytesRead);
			if (bytesRead < 1024)
			{
				_clientsInfo.insert(std::pair<int, Client>(fd, Client(fd, _clientsMsg[fd])));
				_clientsMsg[fd].clear();
				for (size_t i = 0; i < _clientsFd.size(); ++i)
                {
                    if (_clientsFd[i].fd == fd)
                    {
                        _clientsFd[i].events = POLLOUT; //um poll zu sagen, dass requst durch ist und wir ready sind response zu schicken
                        break;
                    }
                }
			}
		}
	}
}

void	Server::response(Client &client)
{
	std::cout << BLUE << "Response" << RESET << std::endl;
	if (client.getResponseBuffer().empty())
	{
		std::string response;
		if (client.getstatusCode() != "200")
			response = handleERROR(client);
		if (client.getMethod() == "GET")
			response = handleGET(client);
		// else if(client.getMethod() == "POST")
		// 	response = handlePOST(&client);
		// else if(client.getMethod() == "DELETE")
		// 	response = handleDELETE(&client);
		// else
		// {
			// response = "";
		// }
		client.setResponseBuffer(response);
		client.getBytesSent() = 0;
	}
	// std::cout << "TEST" << std::endl;

	std::string response = client.getResponseBuffer();
    size_t bytesSent = client.getBytesSent();
    ssize_t remaining = response.size() - bytesSent;

    // Send as much as possible
    ssize_t sent = send(std::stoi(client.getFd()), response.c_str() + bytesSent, remaining, 0);
    if (sent < 0)
    {
        std::cout << RED << "send failed" << RESET << std::endl;
        close(std::stoi(client.getFd()));
        return;
    }
    bytesSent += sent;
    // nachdem alles gesendet wurde, client aud POLLIN einstellen
    if (bytesSent >= response.size())
    {
        for (size_t i = 0; i < _clientsFd.size(); ++i)
        {
            if (_clientsFd[i].fd == std::stoi(client.getFd()))
            {
                _clientsFd[i].events = POLLIN;
                break;
            }
        }
        client.getResponseBuffer().clear(); // Clear buffer fuer naechstes mal
    }
}

std::string	Server::handleERROR(Client &client)
{
	(void)client;
	// std::cout << "Starting ERROR creation" << std::endl;
	// exit(1);
	// std::string	response;
	
	// response = client.getProtocol() + " " + client.getstatusCode() + " " + "NOT FOUND";
	// send(client.getFd(), response.c_str(), )
	// std::cout << response << std::endl;
	return ("Error muss gemacht werder wallah billah habibi");
}

std::string	Server::handleGET(Client &client)
{
	(void)client;
	std::string	html_page = utils::readFile("http/index.html");
	std::string http_response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: " + std::to_string(html_page.size()) + "\r\n"
		"Connection: close\r\n"
		"\r\n" +
		html_page;

	return (http_response);
	// std::cout << "Starting GET creation" << std::endl;
	// t_response response;
	// time_t currentTime;
	// struct tm* ti;

	// time(&currentTime);
	// ti = localtime(&currentTime);
	
	// //start line
	// response.start_line = client.getProtocol() + " " + client.getstatusCode() + "Ok\r\n"; //Ok only if status is 200

	// //Headers
	// response.server_name = "Server: " + this->_config.server_name + "\r\n";
	// response.date = "Date: " + std::string(asctime(ti));
	
	// client.
	// utils::readFile(std::string input)
	
	// response.content_length = "Content-Length: " + ;
	// response.content_type = "Content-Type: " + ;

	// //Body


	// //Empty line
	// response += "\r\n";

	//Body
}