/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/17 13:34:05 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/19 13:33:28 by lglauch          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/server.hpp"

Server::~Server(void)
{
	close(this->socketFd);
	close(this->client);
}

void	Server::initServer(void)
{
	// port sollte bereits in der check_config() hinzugefügt werden
	this->port = 8080;

	this->socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->socketFd == -1)
		throw std::runtime_error("socket failed");

	// addr struct sollte bereits in der check_confing() hinzugefügt werden
	this->addr.sin_family = AF_INET;
	this->addr.sin_addr.s_addr = INADDR_ANY;
	this->addr.sin_port = htons(this->port);

	if (bind(socketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		throw std::runtime_error("bind failed");
}

void	Server::request(void)
{
	socklen_t	len;
	char		requestMsg[10000];
	int			bytesRead;

	if (listen(this->socketFd, 1) < 0)
		throw std::runtime_error("listen failed");

	this->client = accept(this->socketFd, (struct sockaddr *)&this->addr, &len);
	if (this->client < 0)
		throw std::runtime_error("accept failed");

	poll()
	bytesRead = recv(this->client, requestMsg, 10000, 0); // muessen poll dazu beutzen sonst grade 0
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