/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 08:58:47 by lbohm             #+#    #+#             */
/*   Updated: 2025/03/27 16:54:46 by lglauch          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/client.hpp"

Client::Client(int fd, const std::string &msg) : _responseBuffer(""), _bytesSent(0)
{
	std::cout << "Client constuctor called" << std::endl;
	std::stringstream			parse(msg);
	std::vector<std::string>	tmp((std::istream_iterator<std::string>(parse)), std::istream_iterator<std::string>());
	std::string					line;
	size_t						endOfLine;

	_fd = fd;
	_statusCode = "200";
	if (tmp.size() >= 3)
	{
		if (tmp[0] != "GET" && tmp[0] != "POST" && tmp[0] != "DELETE")
			_statusCode = "405";
		else if (tmp[2] != "HTTP/1.1")
			_statusCode = "505";
		else
		{
			_method = tmp[0];
			_path = tmp[1];
			_protocol = tmp[2];
			parse.clear();
			parse.seekg(0);
			std::getline(parse, line);
			while (std::getline(parse, line))
			{
				if (line == "\r" || line.empty())
					break ;
				endOfLine = line.find(":");
				if (endOfLine == std::string::npos)
				{
					_statusCode = "400";
					break ;
				}
				_header.insert(std::pair<std::string, std::string>(line.substr(0, endOfLine), line.substr(endOfLine + 1)));
			}
			parse >> _body;
		}
	}
	else
		_statusCode = "400";
	// std::cout << "fd = " << _fd << std::endl;
	std::cout << "statusCode = " << _statusCode << std::endl;
	std::cout << "method = " << _method << std::endl;
	std::cout << "path = " << _path << std::endl;
	std::cout << "protocol = " << _protocol << std::endl;
	for (auto head = _header.begin(); head != _header.end(); ++head)
	{
		std::cout << head->first << " " << head->second << std::endl;
	}
	std::cout << "body = " << _body << std::endl;
}
