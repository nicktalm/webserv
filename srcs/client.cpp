/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lucabohn <lucabohn@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 08:58:47 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/24 22:11:06 by lucabohn         ###   ########.fr       */
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
	_listen = false;
	_headerReady = false;
	_chunked = false;
	_fd = 0;
	_clientsMsg = "";
	_statusCode = "";
	_method = "";
	_path = "";
	_protocol = "";
	_body = "";
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

std::string	Client::getPath(void)
{
	return (this->_path);
}

void	Client::checkPath(const t_config config)
{
	std::string	fullDir = "", firstDir = "", file = "";

	if (!this->splitPath(fullDir, firstDir, file))
	{
		this->_statusCode = "404";
		return ;
	}
	if (!this->findLocation(config, fullDir)
		|| !this->checkLocation(fullDir, config.root)
		|| !this->checkBodyLimit(config.max_size_server)
		|| !this->checkFile(fullDir, file))
		return ;
	this->_path = fullDir;
	this->_dir = opendir(this->_path.c_str());
	if (!this->_path.empty() && this->_path.find(".py") != std::string::npos)
		this->_exeCGI = true;
}

bool	Client::findLocation(const t_config config, std::string fullDir)
{
	bool	check;
	for (auto loc = config.locations.begin(); loc != config.locations.end(); ++loc)
	{
		if (loc->path == fullDir)
		{
			this->_locationInfo = *loc;
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
	return (true);
}

bool	Client::checkBodyLimit(const long rootMaxSize)
{
	long	tmp = this->_locationInfo.max_size_location;
	long	tmp2 = rootMaxSize;

	if (tmp || tmp2)
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
	_listen = false;
	_headerReady = false;
	_chunked = false;
	_clientsMsg.clear();
	_statusCode = "200";
	_method.clear();
	_path.clear();
	_protocol.clear();
	_body.clear();
	_header.clear();
	_responseHeader = false;
	_responseReady = false;
	_autoIndexPart = 0;
	_bytesSend = 0;
	_responseBuffer.clear();
	_reDirHeader.clear();
	if (_dir != nullptr && closedir(_dir) == -1)
		std::cerr << "closedir failed" << std::endl;
	_dir = nullptr;
	_exeCGI = false;
}