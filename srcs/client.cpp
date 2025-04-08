/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lglauch <lglauch@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: Invalid date        by                   #+#    #+#             */
/*   Updated: 2025/04/08 15:21:58 by lglauch          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <iomanip>
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
	_reDirHeader = "";
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

std::string	Client::getPath(void)
{
	return (this->_path);
}

void	Client::checkPath(const t_config config)
{
	std::string	lastDir = "", firstDir = "", file = "";
	bool		autoIndex = false, reDir = false;

	if (!this->splitPath(lastDir, firstDir, file))
	{
		this->_statusCode = "404";
		return ;
	}
	if (!this->checkLocation(config, firstDir, lastDir, file, autoIndex, reDir))
		return ;
	if (!reDir)
	{
		if (autoIndex)
			this->createAutoIndex(lastDir);
		else
			this->checkFile(lastDir, file);
	}
	this->_path = lastDir + file;
	std::cout << _path << std::endl;
}

bool	Client::checkLocation(const t_config config, const std::string &firstDir, std::string &lastDir, std::string &file, bool &autoindex, bool &reDir)
{
	for (auto loc = config.locations.begin(); loc != config.locations.end(); ++loc)
	{
		if (firstDir == loc->path)
		{
			if (loc->methods.size() > 0 && std::find(loc->methods.begin(), loc->methods.end(), _method) != loc->methods.end())
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
			if (!loc->redir.first.empty() && !loc->redir.second.empty())
			{
				if (loc->redir.first[0] == '3')
				{
					_statusCode = loc->redir.first;
					_reDirHeader = "Location: " + loc->redir.second;
				}
				reDir = true;
			}
			return (true);
		}
	}
	return (_statusCode = "404", false);
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
		bool		isDir;
		struct stat	info;
		std::string	name = openDir->d_name;
		std::string	href;
		std::string	displayName;
		std::string	fileSize;
		std::string	dateChanged;

		if (name == ".")
			continue;
		std::string fullPath = lastDir + name;
		if (stat(fullPath.c_str(), &info) != 0)
			continue;
		isDir = S_ISDIR(info.st_mode);
		href = name + (isDir ? "/" : "");
		displayName = isDir ? "<td class=\"directory\">" : "<td>";
		fileSize = isDir ? "<td>-</td>" : getSize(info.st_size);
		entries << "<tr>\n"
		<< "  " << displayName
		<< "<a href=\"" << href << "\">" << href << "</a></td>"
		<< fileSize << getTime(info.st_mtime) << "\n"
		<< "</tr>\n";
	}
	closedir(dir);
	size_t pos;

	while ((pos = this->_autoIndexBody.find("{{path}}")) != std::string::npos)
		this->_autoIndexBody.replace(pos, 8, lastDir);
	while ((pos = this->_autoIndexBody.find("{{entries}}")) != std::string::npos)
		this->_autoIndexBody.replace(pos, 11, entries.str());
}

std::string	getTime(std::time_t time)
{
	std::tm				*tmp;
	std::stringstream	ret;

	tmp = std::localtime(&time);
	ret << "<td>" << std::put_time(tmp, "%Y-%m-%d %H:%M:%S") << "</td>";
	return (ret.str());
}

std::string	getSize(off_t &bytes)
{
	std::stringstream	ret;
	double				size = bytes;
	std::string			value = " B";

	if (bytes > 1024)
	{
		size = static_cast<double>(bytes) / 1024.0;
		value = " KB";
	}
	else if (bytes > (1024 * 1024))
	{
		size = static_cast<double>(bytes) / (1024.0 * 1024.0);
		value = " MB";
	}
	ret << "<td>" << std::fixed << std::setprecision(1) << size << value << "</td>";
	return (ret.str());
}

bool	Client::splitPath(std::string &fullPath, std::string &firstDir, std::string &file)
{
	size_t	end;

	if (this->getMethod() == "DELETE")
	{
		std::cout << "TEST" << std::endl;
		end = _path.rfind('/');
		fullPath = "/upload/";
		firstDir = "/upload/";
		file = _path.substr(end + 1);
	}
	else
	{
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
	}
	return (true);
}