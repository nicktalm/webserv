/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/20 08:58:47 by lbohm             #+#    #+#             */
/*   Updated: 2025/04/22 15:18:55 by lbohm            ###   ########.fr       */
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
	if (!_headerReady && _clientsMsg.find("\r\n\r\n") != std::string::npos)
		this->headerParsing(fd, config);
	else if (_headerReady)
		_body.append(_clientsMsg);
	else
	{
		_fd = fd;
		_statusCode = "431";
	}
	this->checkBodySize();
	if (_headerReady || _statusCode[0] == '4' || _statusCode[0] == '5')
		_clientsMsg.clear();
	if (_statusCode[0] == '4' || _statusCode[0] == '5')
		this->_listen = false;
}

void	Client::headerParsing(int fd, const t_config config)
{
	std::stringstream			input(_clientsMsg);
	std::vector<std::string>	tmp((std::istream_iterator<std::string>(input)), std::istream_iterator<std::string>());
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
			input.clear();
			input.seekg(0);
			std::getline(input, line);
			while (std::getline(input, line))
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
		}
		_body.append(input.str().substr(input.tellg()));
		this->checkPath(config);
	}
	else
		_statusCode = "404";
	_headerReady = true;
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

void	Client::clear(void)
{
	this->_listen = false;
	this->_headerReady = false;
	this->_clientsMsg.clear();
	this->_statusCode = "200";
	this->_method.clear();
	this->_path.clear();
	this->_protocol.clear();
	this->_body.clear();
	this->_header.clear();
	this->_responseHeader = false;
	this->_responseReady = false;
	this->_autoIndexPart = 0;
	this->_bytesSend = 0;
	this->_responseBuffer.clear();
	this->_reDirHeader.clear();
	if (this->_dir != nullptr && closedir(this->_dir) == -1)
		std::cerr << "closedir failed" << std::endl;
	this->_dir = nullptr;
}