/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/28 14:12:30 by lbohm            ###   ########.fr       */
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
				std::cout << YELLOW << "Client disconnected" << RESET << std::endl;
				close(_clientsFd[it].fd);
				_clientsFd.erase(_clientsFd.begin() + it);
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

void	Server::IO_Error(int bytesRead, std::vector<pollfd>::iterator find)
{
	if (bytesRead == 0)
		std::cout << RED << "Client disconnected" << RESET << std::endl;
	else
	{
		std::cout << RED << "Recv failed" << RESET << std::endl;
		_clientsInfo.erase(find->fd);
		for (auto it = _clientsFd.begin(); it != _clientsFd.end(); ++it)
		{
			if (it->fd == find->fd)
			{
				_clientsFd.erase(it);
				break ;
			}
		}
	}
}

void Server::request(std::vector<pollfd>::iterator pollClient)
{
	char		tmp[1024];

	if (pollClient->fd == _socketFd) //new client trys to connect
	{
		int clientFd = accept(_socketFd, _res->ai_addr, &_res->ai_addrlen);
		if (clientFd < 0)
			std::cout << RED << "Client accept failed" << RESET << std::endl;
		else
		{
			std::cout << BLUE << "New client connected: " << clientFd << RESET << std::endl;
			fcntl(clientFd, F_SETFL, O_NONBLOCK);
			_clientsFd.push_back({clientFd, POLLIN, 0});
			_clientsInfo.emplace(std::piecewise_construct, std::forward_as_tuple(clientFd), std::forward_as_tuple()); // Client wird direkt in die map geschrieben ohen kopie zu erstellen
		}
	}
	else //existing client trys to connect
	{
		int bytesRead = recv(pollClient->fd, tmp, sizeof(tmp), 0);
		if (bytesRead < 0 || (bytesRead == 0 && _clientsInfo[pollClient->fd].getMsg().empty()))
			IO_Error(bytesRead, pollClient);
		else
		{
			_clientsInfo[pollClient->fd].appendMsg(tmp, bytesRead);
			if (bytesRead < 1024)
			{
				_clientsInfo[pollClient->fd].parseRequest(pollClient->fd);
				_clientsInfo[pollClient->fd].clearMsg();
				pollClient->events = POLLOUT;
			}
		}
	}
}

void	Server::response(Client &client, std::vector<pollfd>::iterator pollClient)
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
	ssize_t sent = send(client.getFd(), response.c_str() + bytesSent, remaining, 0);
	if (sent <= 0)
	{
		std::cout << RED << "send failed" << RESET << std::endl;
		close(client.getFd());
		return;
	}
	bytesSent += sent;
	// nachdem alles gesendet wurde, client aud POLLIN einstellen
	if (bytesSent >= response.size())
	{
		pollClient->events = POLLIN;
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