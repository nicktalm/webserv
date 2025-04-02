/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lucabohn <lucabohn@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 08:58:47 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/02 23:15:21 by lucabohn         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include "../include/client.hpp"

Client::Client(void)
{
	_fd = 0;
	_clientsMsg = "";
	_statusCode = "";
	_method = "";
	_path = "";
	_protocol = "";
	_body = "";
	_responseBuffer = "";
	_bytesSent = 0;
}

Client::~Client(void)
{
	close(_fd);
}

void	Client::appendMsg(char *msg, size_t size)
{
	_clientsMsg.append(msg, size);
}

void	Client::parseRequest(int fd)
{
	std::stringstream			parse(_clientsMsg);
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
	_clientsMsg.clear();
}

std::string	Client::getPath(const t_config &config)
{
	if (!_path.empty())
	{
		DIR				*dir;
		struct dirent	*openDir;
		std::string	directory, file;

		size_t	end = _path.rfind('/');
		if (end == std::string::npos)
			return (_statusCode = "404", "");
		directory = _path.substr(0, end + 1);
		file = _path.substr(end + 1);
		if (!this->checkPath(config, directory, file))
			return (_statusCode = "404", "");
		dir = opendir(directory.c_str());
		if (!dir)
			return (_statusCode = "404", "");
		while ((openDir = readdir(dir)) != nullptr)
		{
			if (openDir->d_type == DT_REG && openDir->d_name == file)
			{
				closedir(dir);
				return (directory + file);
			}
		}
		closedir(dir);
	}
	return (_statusCode = "404", "");
}

// TODO muss noch gecheckt werden ob man die methode auf die location angewenden werden darf

bool	Client::checkPath(const t_config config, std::string &dir, std::string &file)
{
	if (dir == "/")
	{
		if (file.empty())
			file = config.index;
		return (dir.insert(0, config.root), true);
	}
	else
	{
		for (auto loc = config.locations.begin(); loc != config.locations.end(); ++loc)
		{
			if (dir == loc->path)
			{
				if (!loc->root.empty())
					dir.insert(0, loc->root);
				else
					dir.insert(0, config.root);
				if (file.empty())
					file = loc->index;
				return (true);
			}
		}
	}
	return (false);
}
