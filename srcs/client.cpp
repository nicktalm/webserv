/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 08:58:47 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/07 17:29:48 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include "../include/client.hpp"
#include "../include/utils.hpp"

Client::Client(void)
{
	_fd = 0;
	_clientsMsg = "";
	_statusCode = "";
	_method = "";
	_path = "";
	_protocol = "";
	_body = "";
	_autoIndexBody = "";
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

void	Client::parseRequest(int fd, const t_config config)
{
	std::stringstream			parse(_clientsMsg);
	std::vector<std::string>	tmp((std::istream_iterator<std::string>(parse)), std::istream_iterator<std::string>());
	std::string					line;
	size_t						endOfLine;

	_fd = fd;
	_statusCode = "200";
	std::cout << "msg" << std::endl;
	std::cout << _clientsMsg << std::endl;
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
			this->checkPath(config);
		}
	}
	else
		_statusCode = "400";
	_clientsMsg.clear();
}

bool	Client::checkLocation(const t_config config, const std::string &firstDir, std::string &lastDir, std::string &file, bool &autoindex)
{
	for (auto loc = config.locations.begin(); loc != config.locations.end(); ++loc)
	{
		if (firstDir == loc->path)
		{
			if (std::find(loc->methods.begin(), loc->methods.end(), _method) != loc->methods.end())
				return (_statusCode = "405", false);
			if (!loc->root.empty())
				lastDir.insert(0, loc->root);
			else
				lastDir.insert(0, config.root);
			if (file.empty())
			{
				if (!loc->index.empty())
					file = loc->index;
				else
				{
					if (loc->autoindex)
						autoindex = true;
				}
			}
			return (true);
		}
	}
	return (_statusCode = "404", false);
}

std::string	Client::getPath(void)
{
	return (this->_path);
}

void	Client::checkPath(const t_config config)
{
	std::string	lastDir = "", firstDir = "", file = "";
	bool		autoIndex = false;

	if (!this->splitPath(lastDir, firstDir, file))
	{
		this->_statusCode = "404";
		return ;
	}
	if (!this->checkLocation(config, firstDir, lastDir, file, autoIndex))
		return ;
	if (autoIndex)
		this->createAutoIndex(lastDir);
	else
		this->checkFile(lastDir, file);
	this->_path = lastDir + file;
	std::cout << "path = " << _path << std::endl;
}

void	Client::checkFile(const std::string &lastDir, const std::string &file)
{
	DIR					*dir;
	struct dirent		*openDir;

	dir = opendir(lastDir.c_str());
	if (!dir)
	{
		this->_statusCode = "404";
		return ;
	}
	while ((openDir = readdir(dir)) != nullptr)
	{
		if (openDir->d_type == DT_REG && openDir->d_name == file)
		{
			closedir(dir);
			return ;
		}
	}
	this->_statusCode = "404";
	closedir(dir);
}

void	Client::createAutoIndex(const std::string &lastDir)
{
	std::stringstream	entries;
	DIR					*dir;
	struct dirent		*openDir;

	this->_autoIndexBody = utils::autoindexTemplate;
	dir = opendir(lastDir.c_str());
	if (!dir)
	{
		this->_statusCode = "404";
		return ;
	}
	while ((openDir = readdir(dir)) != nullptr)
	{
		std::string name = openDir->d_name;

		if (name == ".")
			continue;
		std::string fullPath = lastDir + "/" + name;
		struct stat info;
		if (stat(fullPath.c_str(), &info) != 0)
			continue;
		bool isDir = S_ISDIR(info.st_mode);
		std::string href = name + (isDir ? "/" : "");
		std::string displayName = isDir ? "<td class=\"directory\">" : "<td>";
		// std::string sizeStr = isDir ? "-" : formatSize(info.st_size);
		// std::string modTimeStr = formatTime(info.st_mtime);
		entries << "<tr>"
		<< "<a href=\"" << href << "\">" << href << "</a></td>"
		// << "<td>" << sizeStr << "</td>"
		// << "<td>" << modTimeStr << "</td>"
		<< "</tr>\n";
	}

	size_t pos;

	while ((pos = this->_autoIndexBody.find("{{path}}")) != std::string::npos)
		this->_autoIndexBody.replace(pos, 8, lastDir);
	while ((pos = this->_autoIndexBody.find("{{entries}}")) != std::string::npos)
		this->_autoIndexBody.replace(pos, 11, entries.str());
}

bool	Client::splitPath(std::string &fullPath, std::string &firstDir, std::string &file)
{
	size_t	end;

	end = this->_path.rfind('/');
	if (end == std::string::npos)
		return (false);
	fullPath = this->_path.substr(0, end + 1);
	file = this->_path.substr(end + 1);
	if (end > 0)
	{
		end = this->_path.find('/', 1);
		if (end == std::string::npos)
			firstDir = fullPath;
		firstDir = this->_path.substr(0, end + 1);
	}
	else
	firstDir = fullPath;
	return (true);
}