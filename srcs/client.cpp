/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 08:58:47 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/25 12:34:52 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/client.hpp"

Client::Client(int fd, const std::string &msg)
{
	std::stringstream	header;
	std::stringstream	firstLine;
	std::string			line;
	size_t				headerEnd;
	size_t				end;

	_fd = fd;
	headerEnd = msg.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		_statusCode = "400";
	header << msg.substr(0, headerEnd);
	std::getline(header, line);
	firstLine << line;
	firstLine >> _method >> _path >> _protocol;
	
	while (std::getline(header, line))
	{
		end = line.find(":");
		if (end == std::string::npos)
			_statusCode = "400";
		_header.insert(std::pair<std::string, std::string>(line.substr(0, end), line.substr(end + 1)));
	}

	_body = msg.substr(headerEnd + 4);

	if (_path == "/")
		_path = "index.html";
	else
		_path = "../error_pages/404.html";


	std::cout << "First Line\n{" << std::endl;
	std::cout << RED << "method = " << _method << std::endl;
	std::cout << "path = " << _path << std::endl;
	std::cout << "protocol = " << _protocol << RESET << std::endl;
	std::cout << "}" << std::endl;
	std::cout << std::endl;

	std::cout << "Header\n{" << std::endl;
	for (auto head = _header.begin(); head != _header.end(); ++head)
		std::cout << RED << head->first << " " << head->second << RESET << std::endl;
	std::cout << "}" << std::endl;
	std::cout << std::endl;

	std::cout << "Body\n{" << std::endl;
	std::cout << RED << _body << RESET << std::endl;
	std::cout << "}" << std::endl;
}

// <HTTP-METHODE> <PFAD> <PROTOKOLL>
// Header-Name-1: Wert-1\r\n
// Header-Name-2: Wert-2\r\n
// ...
// Header-Name-N: Wert-N \r\n

// \r\n

// [Optionaler Body, z. B. bei POST]