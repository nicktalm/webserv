/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 08:58:47 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/28 19:53:58 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include "../include/client.hpp"
#include "../include/utils.hpp"

Client::Client(void)
{
	_listen = true;
	_headerReady = false;
	_chunked = false;
	_fd = 0;
	_clientsMsg = "";
	_statusCode = "";
	_method = "";
	_path = "";
	_protocol = "";
	_body = "";
	_query = "";
	_exePath = "";
	_reDirHeader = "";
	_header = {};
}

Client::~Client(void) {}

void	Client::appendMsg(char *msg, size_t size)
{
	_clientsMsg.append(msg, size);
}

void	Client::checkBodySize(void)
{
	std::map<std::string, std::string>::iterator	tmp;
	size_t											size = 0;
	size_t											sizeBody = 0;

	if ((tmp = _header.find("Content-Length")) != _header.end())
	{
		size = std::stoll(tmp->second);
		sizeBody = _body.size();
	}
	if (sizeBody == size)
		this->_listen = false;
	else
		this->_listen = true;
}

void	Client::urlEncoded(void)
{
	std::string	newPath;
	std::string	value;
	int			nbr;

	for (size_t	pos = 0; pos < _path.size(); ++pos)
	{
		if (_path[pos] == '%' && pos < _path.size() - 2)
		{
			value = _path.substr(pos + 1, 2);
			nbr = std::stoi(value, nullptr, 16);
			newPath += static_cast<char>(nbr);
			pos += 2;
			continue;
		}
		newPath += _path[pos];
	}
	_path = newPath;
}

void	Client::queryStr(void)
{
	size_t		pos;
	std::string	newPath;

	pos = _path.find('?');
	if (pos == std::string::npos)
		return ;
	_query = _path.substr(pos + 1);
	newPath = _path.substr(0, pos);
	_path = newPath;
}

void	Client::pathInfo(void)
{
	size_t		pos;
	std::string	newPath;

	pos = _path.find(".py");
	if (pos == std::string::npos)
		return ;
	_pathInfo = _path.substr(pos + 3);
	newPath = _path.substr(0, pos + 3);
	_path = newPath;
}

bool	Client::findLocation(const t_config config, std::string fullDir)
{
	bool	check;
	for (auto loc = config.locations.begin(); loc != config.locations.end(); ++loc)
	{
		if (loc->path == fullDir)
		{
			this->_locationInfo = *loc;
			std::cout << "location = " << loc->path << std::endl;
			return (true);
		}
	}
	check = this->findLocation(config, fullDir.substr(0, fullDir.rfind('/', fullDir.size() - 2) + 1));
	if (!check)
	{
		std::cerr << RED << "Path not found" << RESET << std::endl;
		this->_statusCode = "404";
	}
	return (check);
}

bool	Client::checkLocation(std::string &fullDir, const std::string &mainRoot)
{
	std::vector<std::string>	methods = this->_locationInfo.methods;
	std::pair<std::string, std::string>	redir = this->_locationInfo.redir;

	if (!redir.first.empty())
	{
		if (redir.first[0] == '3' && !redir.second.empty())
		{
			_statusCode = redir.first;
			_reDirHeader = "Location: " + redir.second;
		}
		else
			_statusCode = redir.first;
	}
	else if (methods.size() > 0 && std::find(methods.begin(), methods.end(), this->_method) != methods.end())
		return (_statusCode = "405", false);

	if (!this->_locationInfo.root.empty())
		fullDir.insert(0, this->_locationInfo.root);
	else if (!mainRoot.empty())
		fullDir.insert(0, mainRoot);
	else
		std::cerr << RED << "No Root found" << RESET << std::endl;
	size_t	pos = fullDir.find(_locationInfo.path);
	if (pos != std::string::npos)
		fullDir.erase(pos + 1, _locationInfo.path.size() - 1);
	std::cout << "fullDir in checkLocation = " << fullDir << std::endl;
	return (true);
}

bool	Client::checkBodyLimit(const long rootMaxSize)
{
	long	tmp = this->_locationInfo.max_size_location;
	long	tmp2 = rootMaxSize;

	if (!_chunked && (tmp || tmp2))
	{
		long sizeRequst = 0;
		auto size = _header.find("Content-Length");
		if (size != _header.end())
			sizeRequst = std::stol(size->second);
		if (tmp && tmp < sizeRequst)
			return (_statusCode = "413", false);
		if (tmp2 && tmp2 < sizeRequst)
			return (_statusCode = "413", false);
	}
	return (true);
}

bool	Client::checkFile(std::string &fullDir, std::string file)
{
	struct stat	info;

	std::cout << "fullDir in checkFile = " << fullDir << std::endl;
	std::cout << "file in checkFile = " << file << std::endl;
	if (file.empty() && !this->_locationInfo.autoindex)
	{
		if (!this->_locationInfo.index.empty())
			file = this->_locationInfo.index;
		else
			return (this->_statusCode = "403", false);
	}
	if (access((fullDir + file).c_str(), F_OK))
		return (this->_statusCode = "404", false);
	stat((fullDir + file).c_str(), &info);
	if (!file.empty() && !S_ISREG(info.st_mode))
		return (this->_statusCode = "404", false);
	else if (this->_method == "GET" && (access(fullDir.c_str(), X_OK) || access((fullDir + file).c_str(), R_OK)))
		return (this->_statusCode = "403", false);
	else if ((this->_method == "POST" || this->_method == "DELETE") && access(fullDir.c_str(), X_OK | W_OK))
		return (this->_statusCode = "403", false);
	fullDir.append(file);
	return (true);
}

std::string	Client::createAutoIndex(const std::string &lastDir, const std::string name)
{
	std::stringstream	entrie;
	bool				isDir;
	struct stat			info;
	std::string			href;
	std::string			displayName;
	std::string			fileSize;
	std::string			dateChanged;
	std::string			button;

	if (name == ".")
		return ("");
	std::string fullPath = lastDir + name;
	if (stat(fullPath.c_str(), &info) != 0)
		return ("");
	isDir = S_ISDIR(info.st_mode);
	href = name + (isDir ? "/" : "");
	displayName = isDir ? "<td class=\"directory\">" : "<td>";
	fileSize = isDir ? "<td>-</td>" : getSize(info.st_size);
	button = isDir ? "<td></td>" : "<td><button onclick=\"deleteFile('" + name + "', this)\">Delete</button></td>";


	entrie << "<tr>\n"
	<< "  " << displayName
	<< "<a href=\"" << href << "\">" << href << "</a></td>"
	<< fileSize << getTime(info.st_mtime)
	<< button << "\n"
	<< "</tr>\n";
	return (entrie.str());
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

	if (bytes > 1024 && bytes < (1024 * 1024))
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

	end = _path.rfind('/');
	if (end == std::string::npos)
		return (false);
	fullPath = _path.substr(0, end + 1);
	file = _path.substr(end + 1);
	if (end > 0)
	{
		end = _path.find('/', 1);
		if (end == std::string::npos)
			firstDir = fullPath;
		firstDir = _path.substr(0, end + 1);
	}
	else
		firstDir = fullPath;
	return (true);
}

void	Client::clear(void)
{
	_listen = true;
	_headerReady = false;
	_chunked = false;
	_clientsMsg.clear();
	_statusCode = "200";
	_method.clear();
	_path.clear();
	_protocol.clear();
	_body.clear();
	_query.clear();
	_exePath.clear();
	_header.clear();
	_query.clear();
	_responseHeader = false;
	_responseReady = false;
	_waitForChild = false;
	_autoIndexPart = 0;
	_bytesSend = 0;
	_responseBuffer.clear();
	_reDirHeader.clear();
	if (_dir != nullptr && closedir(_dir) == -1)
		std::cerr << "closedir failed" << std::endl;
	_dir = nullptr;
	_exeCGI = false;
}