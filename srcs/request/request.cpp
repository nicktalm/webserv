/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lbohm <lbohm@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/24 21:23:02 by lucabohn          #+#    #+#             */
/*   Updated: 2025/05/08 00:26:00 by lbohm            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <iterator>
#include "../../include/server.hpp"
#include "../../include/utils.hpp"

void Server::request(std::vector<pollfd>::iterator pollClient)
{
	char		buffer[8192];

	if (pollClient->fd == _socketFd) //new client trys to connect
	{
		int clientFd = accept(_socketFd, nullptr, nullptr);
		if (clientFd < 0)
			std::cerr << RED << "Client accept failed" << RESET << std::endl;
		else
		{
			log(0, "New client connected:", clientFd);
			if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
			{
				std::cerr << RED; perror("fcntl"); std::cerr << RESET;
				return ;
			}
			_clientsFd.push_back({clientFd, POLLIN, 0});
			_clientsInfo.emplace(std::piecewise_construct, std::forward_as_tuple(clientFd), std::forward_as_tuple()); // Client wird direkt in die map geschrieben ohen kopie zu erstellen
		}
	}
	else //existing client trys to connect
	{
		int bytesRead = recv(pollClient->fd, buffer, sizeof(buffer), 0);
		if (bytesRead < 0 || (bytesRead == 0 && _clientsInfo[pollClient->fd].getMsg().empty()))
			IO_Error(bytesRead, pollClient);
		else
		{
			buffer[bytesRead] = '\0';
			_clientsInfo[pollClient->fd].appendMsg(buffer, bytesRead);
			_clientsInfo[pollClient->fd].parseRequest(pollClient->fd, _config);
			if (!_clientsInfo[pollClient->fd].getListen()
						|| (_clientsInfo[pollClient->fd].getStatusCode()[0] == '4' && _clientsInfo[pollClient->fd].getStatusCode() != "413")
						|| _clientsInfo[pollClient->fd].getStatusCode()[0] == '5')
				pollClient->events = POLLOUT;
		}
	}
}

void	Server::IO_Error(int bytesRead, std::vector<pollfd>::iterator find)
{
	if (bytesRead < 0)
		std::cerr << RED << "Recv failed" << RESET << std::endl;
	this->disconnect(find);
}

void	Client::parseRequest(int fd, const t_config config)
{
	if (!_headerReady && _clientsMsg.find("\r\n\r\n") != std::string::npos)
		this->headerParsing(fd, config);
	else if (_headerReady)
	{
		if (!_chunked)
			_body.append(_clientsMsg);
		else
			this->parseChunk(_clientsMsg);
	}
	else if (_clientsMsg.size() == 8192)
	{
		_fd = fd;
		_statusCode = "431";
	}
	if (_headerReady && !_chunked)
		this->checkBodySize();
	if (_headerReady || (_statusCode[0] == '4' && _statusCode != "413") || _statusCode[0] == '5')
		_clientsMsg.clear();
	if ((_statusCode[0] == '4' && _statusCode != "413") || _statusCode[0] == '5')
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
		{
			_statusCode = "405";
			return ;
		}
		else if (tmp[2] != "HTTP/1.1")
		{
			_statusCode = "505";
			return ;
		}
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
		auto encoded = _header.find("Transfer-Encoding");
		if (encoded != _header.end())
		{
			if (encoded->second.find("chunked") != std::string::npos)
				_chunked = true;
		}
		size_t	pos = _clientsMsg.find("\r\n\r\n");
		if (pos != std::string::npos)
		{
			if (!_chunked)
					_body.append(_clientsMsg.substr(pos + 4));
			else
			{
				std::string	chunk = _clientsMsg.substr(pos + 4);

				this->parseChunk(chunk);
			}
		}
		this->checkPath(config);
	}
	else
		_statusCode = "404";
	_headerReady = true;
}

void	Client::printStatus(void)
{
		std::time_t			now = std::time(nullptr);
		std::tm				*local_time = std::localtime(&now);
		std::stringstream	status;
		
		status << std::put_time(local_time, "%H:%M:%S")
				<< " " << _method << " " << _statusCode << " client id: " << _fd;
		std::cout << status.str() << std::endl;
}

void	Client::checkPath(const t_config config)
{
	std::string	fullDir = "", firstDir = "", file = "";

	this->urlEncoded();
	this->queryStr();
	this->pathInfo();
	if (!this->splitPath(fullDir, firstDir, file))
	{
		_statusCode = "404";
		return ;
	}
	if (!this->findLocation(config, fullDir)
		|| !this->checkLocation(fullDir, config.root)
		|| !this->checkBodyLimit(config.max_size_server)
		|| !this->checkFile(fullDir, file))
		return ;
	_path = fullDir;
	_dir = opendir(_path.c_str());
	size_t	pos =  _path.rfind('.');
	if (pos != std::string::npos)
	{
		std::string	tmp = _path.substr(pos);

		for (auto it = config.locations.begin(); it != config.locations.end(); ++it)
		{
			if (tmp == it->path)
			{
				_exeCGI = true;
				break ;
			}
		}
	}
}

void	Client::parseChunk(std::string chunk)
{
	while (true)
	{
		size_t		posValue = chunk.find("\n");
		std::string	tmp;
		std::string	value;
		int			size;

		if (posValue == std::string::npos)
			break ;
		value = chunk.substr(0, posValue);
		size = std::stoi(value, nullptr, 16);
		if (size == 0)
		{
			_listen = false;
			break ;
		}
		tmp = chunk.substr(posValue + 1);
		int	i = 0;
		int	end = 0;
		while (i < size)
		{
			if (tmp[end] != '\r' && tmp[end] != '\n')
				++i;
			++end;
		}
		++end;
		_body.append(tmp.substr(0, end));
		chunk = tmp.substr(end + 1);
	}
}