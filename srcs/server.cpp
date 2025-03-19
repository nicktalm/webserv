/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/19 17:32:59 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"

Server::Server(t_config config) : _config(config)
{
	std::cout << GREEN << "Starting server on port " << config._port << RESET << std::endl;

	_socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFd == -1)
		throw std::runtime_error("socket failed");

	// addr struct sollte bereits in der check_confing() hinzugefügt werden
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = INADDR_ANY;
	_addr.sin_port = htons(_config._port);

	if (bind(_socketFd, (struct sockaddr *)&_addr, sizeof(_addr)) < 0)
		throw std::runtime_error("bind failed");
}

Server::~Server(void)
{
	close(_socketFd);
}

void	Server::initServer(void)
{

}

void	Server::request(void)
{
	socklen_t	len;
	char		requestMsg[10000];
	int			bytesRead;

	if (listen(_socketFd, 1) < 0)	
		throw std::runtime_error("listen failed");

	_clients[0].setFd() = accept(_socketFd, (struct sockaddr *)&_addr, &len);
	if (_clients[0]._Fd < 0)
		throw std::runtime_error("accept failed");

	// poll()
	bytesRead = recv(_clients[0]._Fd, requestMsg, 10000, 0); // muessen poll dazu beutzen sonst grade 0
	if (bytesRead < 0)
		throw std::runtime_error("recv failed");
	else if (bytesRead == 0)
		throw std::runtime_error("recv no content");

	std::cout << "Msg:" << std::endl;
	std::cout << requestMsg << std::endl;
}

void	Server::response(void)
{
	std::string	html_page = readFile("index.html");

	std::string http_response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: " + std::to_string(html_page.size()) + "\r\n"
		"Connection: close\r\n"
		"\r\n" +
		html_page;

	int bytesSend = send(this->client, http_response.c_str(), http_response.length(), 0); //muessen poll dazu beutzen sonst grade 0
	if (bytesSend == 0)
		throw std::runtime_error("send is empty");
	else if (bytesSend < 0)
		throw std::runtime_error("send failed");
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