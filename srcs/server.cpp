/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/04 11:54:01 by lglauch          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <netdb.h>
#include <sstream>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include "../include/response.hpp"
#include "../include/utils.hpp"
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
	if (eventCount < 0 && runner)
		throw std::runtime_error("Poll failed");
	else if (eventCount > 0)
	{
		for (size_t it = 0; it < _clientsFd.size();)
		{
			if (_clientsFd[it].revents & POLLHUP) //client disconnected or hung up
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

void	Server::IO_Error(int bytesRead, std::vector<pollfd>::iterator find)
{
	if (bytesRead < 0)
		std::cerr << RED << "Recv failed" << RESET << std::endl;
	this->disconnect(find);
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
				pollClient->events = POLLOUT;
			}
		}
	}
}

std::string Server::handlePOST(Client &client)
{
	std::cout << PURPLE << "PostRequest" << RESET << std::endl;

	std::cout << "Received Body: " << client.getBody() << std::endl;

	// Beispiel: Speichere den Body in einer Datei
	std::ofstream outFile("uploaded_data.txt");
	if (!outFile)
		return "HTTP/1.1 500 Internal Server Error\r\n\r\n";

	outFile << client.getBody(); // Schreibe den Body in die Datei
	outFile.close();

	// Generiere eine Antwort
	std::string response = "HTTP/1.1 201 Created\r\n";
	response += "Content-Length: 0\r\n";
	response += "Connection: close\r\n\r\n";

	return response;
}

void	Server::response(Client &client, std::vector<pollfd>::iterator pollClient)
{
	std::cout << BLUE << "Response" << RESET << std::endl;
	if (client.getResponseBuffer().empty())
	{
		std::string response;
		if (client.getstatusCode() != "200")
			response = handleERROR(client);
		else if (client.getMethod() == "GET")
			response = handleGET(client);
		else if(client.getMethod() == "POST")
			response = handlePOST(client);
		else if(client.getMethod() == "DELETE")
			response = handleDELETE(client);
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
	Response	repo;
	t_response	response;
	std::string	errorMsg;
	std::string	path;
	size_t		end;

	errorMsg = repo.getErrorMsg(client.getstatusCode());
	end = errorMsg.find(':');
	path = errorMsg.substr(end + 2);
	if (utils::readFile(path, response.body) == false)
		return (client.setStatusCode(404), handleERROR(client));
	response.start_line = "HTTP/1.1 " + client.getstatusCode() + " " + errorMsg.substr(0, end);
	response.server_name = "Servername: " + this->_config.server_name;
	response.date = "Date: " + utils::getDate();
	response.content_length = "Content-Length: " + std::to_string(response.body.size());
	response.content_type = "Content-Type: " + repo.getContentType(path);
	
	return (Server::create_response(response));
}

std::string Server::handleDELETE(Client &client)
{
	std::string path = client.getPath(_config);
}

// TODO man muss noch check ob die error page vorhanden ist in der config file

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

std::string	Server::handleGET(Client &client)
{
	std::cout << PURPLE << "GetRequest" << RESET << std::endl;
	Response responseInstance;
	t_response response;
	std::string	path;
	
	path = client.getPath(this->_config);
	if (path.empty())
		return (handleERROR(client));
	if (utils::readFile(path, response.body) == false)
		return (client.setStatusCode(404), handleERROR(client));
	response.server_name = "Servername: " + this->_config.server_name;
	response.date = "Date: " + utils::getDate();
	response.content_length = "Content-Length: " + std::to_string(response.body.size());
	response.content_type = "Content-Type: " + responseInstance.getContentType(path);
	response.start_line = responseInstance.getStartLine(client.getProtocol(), client.getstatusCode());
	return (Server::create_response(response));
}

void	Server::disconnect(std::vector<pollfd>::iterator find)
{
	std::cout << YELLOW << "Client disconnected: " << find->fd << RESET << std::endl;
	auto del = _clientsInfo.find(find->fd);
	_clientsInfo.erase(del);
	close(find->fd);
	_clientsFd.erase(find);
}
